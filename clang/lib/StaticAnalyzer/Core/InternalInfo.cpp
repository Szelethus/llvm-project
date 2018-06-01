//===- InternalInfo.cpp -----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines implementations of InternalInfo.
//
//===----------------------------------------------------------------------===//

#include "InternalInfoImplementations.h"
#include "clang/Basic/PlistSupport.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/RangedConstraintManager.h"

using namespace clang;
using namespace ento;
using namespace markup;

//===----------------------------------------------------------------------===//
// InternalInfoMap methods.
//===----------------------------------------------------------------------===//

void InternalInfoMap::addFIDs(FIDMap &FIDs, SmallVectorImpl<FileID> &V,
                              const SourceManager &SM) const {
  for (auto &E : Set)
    E.addFIDs(FIDs, V, SM);
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
LLVM_DUMP_METHOD void
InternalInfoMap::dumpToStream(llvm::raw_ostream &Out,
                              const SourceManager &SM) const {
  for (auto &E : Set) {
    E.dumpToStream(Out, SM);
    Out << '\n';
  }
}
#endif

//===----------------------------------------------------------------------===//
// ConstraintInfo methods.
//===----------------------------------------------------------------------===//

void ConstraintInfo::ConstraintEntry::update(const ConstraintEntry &Other) {
  if (Statement.empty()) {
    Statement = Other.Statement;
  }
  if (Symbol.empty()) {
    Symbol = Other.Symbol;
  }
  if (Constraint.empty()) {
    Constraint = Other.Constraint;
  }
}

void ConstraintInfo::copyContents(const ConstraintInfo &Other) {
  for (const ConstraintMapTy::value_type &Entry : Other.ConstraintMap) {
    ConstraintMap.insert(Entry);
  }
}

namespace {

class GetAllConcreteValues : public StoreManager::BindingsHandler {
  using ConstraintMapTy = ConstraintInfo::ConstraintMapTy;
  using ConstraintEntry = ConstraintInfo::ConstraintEntry;

  ConstraintMapTy &ConstraintMap;
  ProgramStateRef State;

public:
  GetAllConcreteValues(ConstraintMapTy &C, ProgramStateRef State)
      : ConstraintMap(C), State(State) {}

  virtual bool HandleBinding(StoreManager &SMgr, Store Store,
                             const MemRegion *R, SVal V) {
    addToMapIfConcreteInt<loc::ConcreteInt>(R, V);
    addToMapIfConcreteInt<nonloc::ConcreteInt>(R, V);
    return true;
  }

private:
  template <class LocOrNonlocConcreteInt>
  void addToMapIfConcreteInt(const MemRegion *R, SVal V) {
    static_assert(
        std::is_same<LocOrNonlocConcreteInt, loc::ConcreteInt>::value ||
            std::is_same<LocOrNonlocConcreteInt, nonloc::ConcreteInt>::value,
        "Template parameter may only be a loc::ConcreteInt "
        "or nonloc::ConcreteInt!");

    const auto &C = V.getAs<LocOrNonlocConcreteInt>();
    if (!C)
      return;

    if (R->sourceRange().isInvalid())
      return;

    ConstraintEntry CEntry;

    llvm::raw_string_ostream StmtOS(CEntry.Statement);
    llvm::raw_string_ostream ConstraintOS(CEntry.Constraint);

    R->dumpToStream(StmtOS);
    ConstraintOS << " == ";

    printValueOfConcreteInt(*C, ConstraintOS);
    ConstraintOS.flush();
    StmtOS.flush();

    ConstraintMap[R->sourceRange()].update(CEntry);
  }

  void printValueOfConcreteInt(loc::ConcreteInt C, llvm::raw_ostream &Out) {
    if (C.isZeroConstant())
      Out << "nullptr";
    else
      Out.write_hex(C.getValue().getExtValue());
  }

  void printValueOfConcreteInt(nonloc::ConcreteInt C, llvm::raw_ostream &Out) {
    Out << C.getValue();
  }
};

} // end of anonymous namespace

void ConstraintInfo::AddEnvironmentConstraintsToMap(ProgramStateRef State) {
  const Environment &Env = State->getEnvironment();

  for (const std::pair<EnvironmentEntry, SVal> &Entry : Env) {

    const Stmt *EntryStmt = Entry.first.getStmt();
    if (EntryStmt->getSourceRange().isInvalid())
      continue;

    const SVal &EntryVal = Entry.second;
    ConstraintEntry CEntry;

    llvm::raw_string_ostream StmtOS(CEntry.Statement);

    EntryStmt->printPretty(
        StmtOS, /*PrinterHelper*/ nullptr,
        PrintingPolicy(State->getStateManager().getContext().getLangOpts()));
    StmtOS.flush();

    // If this entry is a binary operator, like 'i < 0' or 'j + 10'.
    if (const auto &BinOp = dyn_cast_or_null<BinaryOperator>(EntryStmt)) {

      // If we know the value of the binary expression, add it to the map.
      const auto &Concr = EntryVal.getAs<nonloc::ConcreteInt>();
      if (!Concr)
        continue;

      llvm::raw_string_ostream ConstraintOS(CEntry.Constraint);

      ConstraintOS << " == ";
      // If this is a binary operator that returns with a boolean value
      // (<, >, ==, etc).
      if (BinOp->getType()->isBooleanType()) {
        if (Concr->getValue().isNullValue())
          ConstraintOS << "false";
        else
          ConstraintOS << "true";
      } else {
        // This is a binary operator that returns with a non-boolean value
        // (+, -, <<, etc).
        ConstraintOS << Concr->getValue().getExtValue();
      }
      ConstraintOS.flush();

      // This entry tells us that the Entry was assigned a symbol like 'reg_$0'.
    } else if (const SymbolRef Sym = EntryVal.getAsSymbol()) {
      llvm::raw_string_ostream SymbolOS(CEntry.Symbol);

      Sym->dumpToStream(SymbolOS);

      SymbolOS.flush();

      // Any other entries are of no use to us, discard them.
    } else {
      continue;
    }

    ConstraintMap[EntryStmt->getSourceRange()].update(CEntry);
  }
}

void ConstraintInfo::AddConstraintManagerConstraintsToMap(
    ProgramStateRef State) {

  // Adding constraints from RangeConstraintManager.
  const ConstraintRangeTy &RangeConstraints = State->get<ConstraintRange>();
  for (const std::pair<SymbolRef, RangeSet> &Pair : RangeConstraints) {

    const MemRegion *OrigR = Pair.first->getOriginRegion();
    if (!OrigR)
      continue;

    if (OrigR->sourceRange().isInvalid())
      continue;

    ConstraintEntry CEntry;

    llvm::raw_string_ostream ConstraintOS(CEntry.Constraint);

    if (CEntry.Constraint.empty()) {
      llvm::raw_string_ostream SymbolOS(CEntry.Symbol);
      Pair.first->dumpToStream(SymbolOS);
    }
    ConstraintOS << " in ";
    Pair.second.print(ConstraintOS);
    ConstraintOS.flush();

    ConstraintMap[OrigR->sourceRange()].update(CEntry);
  }
}

void ConstraintInfo::addConstraintsFromState(ProgramStateRef State) {
  // Collecting constraints and symbols from the environment (such as i < 0).
  AddEnvironmentConstraintsToMap(State);

  // Collecting information about nonloc::ConcreteInt and loc::ConcreteInt
  // values from the store.
  GetAllConcreteValues Callback(ConstraintMap, State);
  State->getStateManager().getStoreManager().iterBindings(State->getStore(),
                                                          Callback);

  // Collecting constraints from constraint managers.
  AddConstraintManagerConstraintsToMap(State);
}

static void printSourceRange(llvm::raw_ostream &Out, const SourceRange &R,
                             const SourceManager &SM) {
  Out << "start: ";
  R.getBegin().print(Out, SM);
  Out << " end: ";
  R.getEnd().print(Out, SM);
}

void ConstraintInfo::addFIDs(FIDMap &FIDs, SmallVectorImpl<FileID> &V,
                             const SourceManager &SM) const {
  assert(std::find_if(ConstraintMap.begin(), ConstraintMap.end(),
                      [](const ConstraintMapTy::value_type &Pair) {
                        return Pair.first.isInvalid();
                      }) == ConstraintMap.end() &&
         "ConstraintInfo stored an entry with an invalid SourceRange!");
  for (const ConstraintMapTy::value_type &Pair : ConstraintMap) {
    AddFID(FIDs, V, SM, Pair.first.getBegin());
    AddFID(FIDs, V, SM, Pair.first.getEnd());
  }
}

void ConstraintInfo::printPlist(llvm::raw_ostream &Out, const SourceManager &SM,
                                const markup::FIDMap &FM,
                                unsigned indent) const {
  Indent(Out, indent) << "<dict>\n";
  ++indent;

  Indent(Out, indent) << "<key>kind</key><string>constraintinfo</string>\n";
  Indent(Out, indent) << "<key>entries</key>\n";
  Indent(Out, indent) << "<array>\n";
  ++indent;

  for (const ConstraintMapTy::value_type &Pair : ConstraintMap) {
    Indent(Out, indent) << "<dict>\n";
    ++indent;

    const ConstraintEntry &Entry = Pair.second;

    Indent(Out, indent) << "<key>location</key>\n";
    EmitRange(Out, SM, {Pair.first, false}, FM, indent);

    Indent(Out, indent) << "<key>statement</key>\n";
    Indent(Out, indent);
    EmitString(Out, Entry.Statement) << '\n';

    Indent(Out, indent) << "<key>symbol</key>\n";
    Indent(Out, indent);
    EmitString(Out, Entry.Symbol) << '\n';

    Indent(Out, indent) << "<key>constraint</key>\n";
    Indent(Out, indent);
    EmitString(Out, Entry.Constraint) << '\n';

    --indent;
    Indent(Out, indent) << "</dict>\n";
  }

  --indent;
  Indent(Out, indent) << "</array>\n";

  --indent;
  Indent(Out, indent) << "</dict>\n";
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
LLVM_DUMP_METHOD void
ConstraintInfo::dumpToStream(llvm::raw_ostream &Out,
                             const SourceManager &SM) const {
  Out << "ConstraintInfo entries:\n";
  for (const ConstraintMapTy::value_type &Pair : ConstraintMap) {
    Out << '\n';

    const ConstraintEntry &Entry = Pair.second;
    printSourceRange(Out, Pair.first, SM);
    const std::string NewLine = "\n  ";
    Out << NewLine << "statement: " << Entry.Statement << NewLine
        << "symbol: " << Entry.Symbol << NewLine
        << "constraint: " << Entry.Constraint << '\n';
  }
}
#endif

//===----------------------------------------------------------------------===//
// StateInfo methods.
//===----------------------------------------------------------------------===//

void StateInfo::addStateInfoFromState(ProgramStateRef State) {
  llvm::raw_string_ostream OS(StateDump);
  State->printJson(OS);
}

void StateInfo::printPlist(llvm::raw_ostream &Out, const SourceManager &SM,
                           const markup::FIDMap &FM, unsigned indent) const {
  Indent(Out, indent) << "<dict>\n";
  ++indent;

  Indent(Out, indent) << "<key>kind</key><string>stateinfo</string>\n";
  Indent(Out, indent) << "<key>entry</key>\n";
  Indent(Out, indent);
  EmitString(Out, StateDump) << '\n';

  --indent;
  Indent(Out, indent) << "</dict>\n";
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
LLVM_DUMP_METHOD void StateInfo::dumpToStream(llvm::raw_ostream &Out,
                                              const SourceManager &SM) const {
  Out << "StateInfo entries:\n";
  Out << StateDump << '\n';
}
#endif
