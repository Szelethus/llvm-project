//===- InternalInfoImplementations.h ----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the InternalInfo interfaces.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CORE_INTERNALINFOIMPLEMENTATIONS_H
#define LLVM_CLANG_STATICANALYZER_CORE_INTERNALINFOIMPLEMENTATIONS_H

#include "clang/StaticAnalyzer/Core/BugReporter/InternalInfo.h"

namespace clang {
namespace ento {

class ConstraintInfo : public InternalInfo {
public:
  struct ConstraintEntry {
    std::string Statement;
    std::string Symbol;
    std::string Constraint;

    ConstraintEntry() = default;

    /// Overwrites all empty string fields from \p Other.
    void update(const ConstraintEntry &Other);
  };

  struct SourceRangeComparator {
    bool operator()(const SourceRange &LHS, const SourceRange &RHS) const {
      if (LHS.getBegin() == RHS.getBegin()) {
        return LHS.getEnd() < RHS.getEnd();
      } else {
        return LHS.getBegin() < RHS.getBegin();
      }
    }
  };

  using ConstraintMapTy =
      std::map<SourceRange, ConstraintEntry, SourceRangeComparator>;

private:
  ConstraintMapTy ConstraintMap;

public:
  ConstraintInfo() : InternalInfo(Constraint) {}

  static constexpr Kind getStaticKind() { return Constraint; }

  void copyContents(const ConstraintInfo &Other);

  void addConstraintsFromState(ProgramStateRef State);

  virtual void addFIDs(markup::FIDMap &FIDs, SmallVectorImpl<FileID> &V,
                       const SourceManager &SM) const override;

  virtual void printPlist(llvm::raw_ostream &Out, const SourceManager &SM,
                          const markup::FIDMap &FM,
                          unsigned Indent) const override;

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  LLVM_DUMP_METHOD
  virtual void dumpToStream(llvm::raw_ostream &Out,
                            const SourceManager &SM) const override;
#endif

private:
  void AddEnvironmentConstraintsToMap(ProgramStateRef State);

  void AddConstraintManagerConstraintsToMap(ProgramStateRef State);
};

class StateInfo : public InternalInfo {
  std::string StateDump;

public:
  StateInfo() : InternalInfo(State) {}

  static constexpr Kind getStaticKind() { return State; }

  void addStateInfoFromState(ProgramStateRef State);

  bool isEmpty() { return StateDump.empty(); }

  virtual void addFIDs(markup::FIDMap &FIDs, SmallVectorImpl<FileID> &V,
                       const SourceManager &SM) const override {}

  virtual void printPlist(llvm::raw_ostream &Out, const SourceManager &SM,
                          const markup::FIDMap &FM,
                          unsigned Indent) const override;

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  LLVM_DUMP_METHOD
  virtual void dumpToStream(llvm::raw_ostream &Out,
                            const SourceManager &SM) const override;
#endif
};

} // namespace ento
} // namespace clang

#endif // LLVM_CLANG_STATICANALYZER_CORE_BUGREPORTER_INTERNALINFO_H
