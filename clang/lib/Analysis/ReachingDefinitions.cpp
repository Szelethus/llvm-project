//===--- ReachableDefinitions.cpp -------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Calculates reachable definitions for a variable.
//
//===----------------------------------------------------------------------===//

#include "clang/Analysis/Analyses/ReachingDefinitions.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclLookups.h"
#include "clang/AST/Expr.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersInternal.h"
#include "clang/Analysis/CFG.h"
#include "clang/Basic/LLVM.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/SVals.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SetOperations.h"
#include "llvm/Support/ErrorHandling.h"
#include <memory>

using namespace clang;
using namespace ReachingDefinitionsDetail;
using namespace ast_matchers;

using DefinitionKind = Definition::DefinitionKind;

//===----------------------------------------------------------------------===//
// Utility functions.
//===----------------------------------------------------------------------===//

/// Print a field chain, e.g. 's.x.y'. Mind that even the indirection operator
/// (->) will be written as a dot, but that shouldn't really be an issue since
/// we don't argue about pointees.
static void dumpFieldChain(const FieldChainTy &FieldChain) {
  for (const FieldDecl *Field : FieldChain)
    llvm::errs() << "." + Field->getNameAsString();
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
LLVM_DUMP_METHOD static void dumpFieldChainNL(const FieldChainTy &FieldChain) {
  dumpFieldChain(FieldChain);
  llvm::errs() << '\n';
}
#endif

/// The reaching definitions calculator implemented here uses the CFG and some
/// AST analysis and does not employ points-to analysis, so we're ignoring
/// any field that is indirectly contained.
static bool hasIndirection(FieldChainTy FieldChain) {
  if (FieldChain.empty())
    return false;

  FieldChainTy::iterator It = llvm::find_if(FieldChain, [](const FieldDecl *F) {
    return ento::Loc::isLocType(F->getType());
  });

  // Yay, no indirection in the chain.
  if (It == FieldChain.end())
    return false;

  // If the last element in the chain is a LocType, we're not arguing about
  // aliasing, only the value of a pointer object. That is fine.
  if (*It == FieldChain.back())
    return false;

  return true;
}

/// Visit each field of a 'variable', and its subfields recursively, and call
/// the callback on each.
///
///   \param [in] CB A callback that expects a single FieldChain object.
template <class CallBack>
static void forAllFields(const VarDecl *Var, FieldChainTy FieldChain,
                         CallBack CB) {
  if (hasIndirection(FieldChain))
    return;

  const RecordDecl *R = nullptr;
  if (FieldChain.empty())
    R = Var->getType()->getAsRecordDecl();
  else
    R = FieldChain.back()->getType()->getAsRecordDecl();

  CB(FieldChain);

  if (R) {
    for (const FieldDecl *Field : R->fields()) {
      FieldChainTy Cpy = FieldChain;
      Cpy.emplace_back(Field);
      forAllFields(Var, Cpy, CB);
    }
  }
}

static bool killsVar(const Definition &Victim, const GenSet &Set) {
  for (const Definition &Perpetrator : Set)
    if (std::make_pair(Victim.Var, Victim.FieldChain) ==
        std::make_pair(Perpetrator.Var, Perpetrator.FieldChain))
      return true;
  return false;
}

static StringRef describeDefinitionKind(DefinitionKind K) {
  switch (K) {
  case Definition::Write:
    return "write";
  case Definition::Invalidation:
    return "invalidation";
  }
}

//===----------------------------------------------------------------------===//
// Methods of Definition.
//===----------------------------------------------------------------------===//

bool operator<(const FieldChainTy &Lhs, const FieldChainTy &Rhs) {
  if (Lhs.size() != Rhs.size())
    return Lhs.size() < Rhs.size();

  for (size_t Index = 0; Index < Lhs.size(); ++Index) {
    if (Lhs[Index] != Rhs[Index])
      return Lhs[Index] < Rhs[Index];
  }

  return false;
}

void Definition::dump() const {
  llvm::errs() << "(" << Var->getNameAsString();
  dumpFieldChain(FieldChain);

  llvm::errs() << ", [" << getCFGBlock()->getIndexInCFG() << ", "
               << E.getIndexInBlock() << "])"
               << " <" << (describeDefinitionKind(Kind)) << ">";
}

//===----------------------------------------------------------------------===//
// Matcher callbacks for constructing GEN sets for the variable finding stage.
//===----------------------------------------------------------------------===//

class VariableCollectorCB : public GenSetMatcherCallback {
public:
  VariableCollectorCB(GenSetBuilder &GSBuilder)
      : GenSetMatcherCallback(GSBuilder) {}

  static internal::Matcher<Stmt> getMatcher() {
    return stmt(forEachDescendant(declRefExpr().bind("var")));
  }

  virtual void run(const MatchFinder::MatchResult &Result) override {
    const auto *E = Result.Nodes.getNodeAs<DeclRefExpr>("var");
    assert(E);
    if (const VarDecl *Var = dyn_cast<VarDecl>(E->getDecl()))
      GSBuilder.addVariable(Var, {});
  }
};

//===----------------------------------------------------------------------===//
// Matcher callbacks for constructing GEN sets for the definition finding stage.
//===----------------------------------------------------------------------===//

class AssignmentOperatorCB : public GenSetMatcherCallback {
public:
  AssignmentOperatorCB(GenSetBuilder &GSBuilder)
      : GenSetMatcherCallback(GSBuilder) {}

  static internal::Matcher<Stmt> getMatcher() {
    return binaryOperator(isAssignmentOperator()).bind("assign");
  }

  virtual void run(const MatchFinder::MatchResult &Result) override {
    const auto *BO = Result.Nodes.getNodeAs<BinaryOperator>("assign");
    assert(BO);
    GSBuilder.handleExpr(BO->getLHS(), DefinitionKind::Write);
  }
};

class DeclarationCB : public GenSetMatcherCallback {
public:
  DeclarationCB(GenSetBuilder &GSBuilder) : GenSetMatcherCallback(GSBuilder) {}

  static internal::Matcher<Stmt> getMatcher() {
    return declStmt().bind("decls");
  }

  virtual void run(const MatchFinder::MatchResult &Result) override {
    const auto *DS = Result.Nodes.getNodeAs<DeclStmt>("decls");
    assert(DS);

    for (const Decl *D : DS->decls()) {
      const auto *Var = dyn_cast<VarDecl>(D);
      if (!Var)
        continue;

      GSBuilder.insertToGenSet(Var, DefinitionKind::Write);
    }
  }
};

class CallExprCB : public GenSetMatcherCallback {
public:
  CallExprCB(GenSetBuilder &GSBuilder) : GenSetMatcherCallback(GSBuilder) {}

  static internal::Matcher<Stmt> getMatcher() {
    return callExpr().bind("calls");
  }

  static QualType stripPointers(QualType T) {
    while (!T->getPointeeType().isNull())
      T = T->getPointeeType();

    return T;
  }

  static bool isPointerToNonConstTy(QualType T) {
    if (T->getPointeeType().isNull())
      return false;
    return !stripPointers(T).isConstQualified();
  }

  virtual void run(const MatchFinder::MatchResult &Result) override {
    const auto *Call = Result.Nodes.getNodeAs<CallExpr>("calls");
    assert(Call);

    GSBuilder.invalidateGlobals();

    const FunctionDecl *FD = Call->getDirectCallee();
    if (!FD)
      return;
    bool HasImplicitThisParam = isa<CXXMethodDecl>(FD);

    for (size_t Index = 0,
                E = Call->getNumArgs() - (HasImplicitThisParam ? 1 : 0);
         Index < E; ++Index) {
      QualType ParamTy = FD->getParamDecl(Index)->getOriginalType();
      if (!ParamTy->isRecordType() && !isPointerToNonConstTy(ParamTy))
        continue;
      GSBuilder.handleExpr(Call->getArg(Index + (HasImplicitThisParam ? 1 : 0)),
                           Definition::Invalidation);
    }
  }
};

//===----------------------------------------------------------------------===//
// Matcher callbacks for constructing GEN sets for the expression finding stage.
//===----------------------------------------------------------------------===//

class DeclRefExprCB : public GenSetMatcherCallback {
public:
  DeclRefExprCB(GenSetBuilder &GSBuilder) : GenSetMatcherCallback(GSBuilder) {}

  static internal::Matcher<Stmt> getMatcher() {
    return declRefExpr(to(varDecl().bind("var")));
  }

  virtual void run(const MatchFinder::MatchResult &Result) override {
    const auto *Var = Result.Nodes.getNodeAs<VarDecl>("var");
    assert(Var);
    GSBuilder.insertToGenSet(Var);
  }
};

class ParenExprCB : public GenSetMatcherCallback {
public:
  ParenExprCB(GenSetBuilder &GSBuilder) : GenSetMatcherCallback(GSBuilder) {}

  static internal::Matcher<Stmt> getMatcher() {
    return parenExpr().bind("paren");
  }

  virtual void run(const MatchFinder::MatchResult &Result) override {
    const auto *Paren = Result.Nodes.getNodeAs<ParenExpr>("paren");
    assert(Paren);
    // TODO
  }
};

class MemberExprCB : public GenSetMatcherCallback {
public:
  MemberExprCB(GenSetBuilder &GSBuilder) : GenSetMatcherCallback(GSBuilder) {}

  static internal::Matcher<Stmt> getMatcher() {
    return memberExpr(unless(hasAncestor(memberExpr()))).bind("member");
  }

  virtual void run(const MatchFinder::MatchResult &Result) override {
    // If we found an assignemnt to S.x.y.z, the matcher will match for
    // the last MemberExpr (z), let's build the fieldchain back up.
    const auto *Member = Result.Nodes.getNodeAs<MemberExpr>("member");
    assert(Member);
    FieldChainTy FieldChain;
    FieldChain.emplace_back(cast<FieldDecl>(Member->getMemberDecl()));

    const Expr *NextBase = Member->getBase()->IgnoreParenCasts();
    // Retrieve the next field in the fieldchain.
    // FIXME: How about 'a.x.getCookie().x'?
    while ((Member = dyn_cast<MemberExpr>(NextBase))) {
      FieldChain.emplace_back(cast<FieldDecl>(Member->getMemberDecl()));
      NextBase = Member->getBase()->IgnoreParenCasts();
    }
    std::reverse(FieldChain.begin(), FieldChain.end());

    GSBuilder.insertToGenSet(
        cast<VarDecl>(cast<DeclRefExpr>(NextBase)->getDecl()), FieldChain);
  }
};

//===----------------------------------------------------------------------===//
// Methods of GenSetBuilder.
//===----------------------------------------------------------------------===//

GenSetBuilder::GenSetBuilder(const Decl *D)
    : D(D), Context(&D->getASTContext()) {

  // TODO: Should we match the entire TU for nested static variables?
  addMatcher<&GenSetBuilder::VariableFinder, VariableCollectorCB>();

  // TODO: Elvis operator (?:).
  addMatcher<&GenSetBuilder::ExpressionFinder, DeclRefExprCB>();
  addMatcher<&GenSetBuilder::ExpressionFinder, MemberExprCB>();
  addMatcher<&GenSetBuilder::ExpressionFinder, ParenExprCB>();

  // TODO: Destructor calls? Should we be THAT conservative?
  // TODO: Moving an object?
  // TODO: Method calls?
  // TODO: Analyzing a method?
  // TODO: What do you do with Objective.*???
  // TODO: Exceptions?
  addMatcher<&GenSetBuilder::DefinitionFinder, AssignmentOperatorCB>();
  addMatcher<&GenSetBuilder::DefinitionFinder, DeclarationCB>();
  addMatcher<&GenSetBuilder::DefinitionFinder, CallExprCB>();

  // Collect all used variables.
  if (const auto *FD = dyn_cast<FunctionDecl>(D)) {
    VariableFinder.match(*FD, FD->getASTContext());
    for (const ParmVarDecl *Param : FD->parameters())
      addVariable(Param, {});
  }

  // Collect all visible, non-local variables in a separate set.
  // TODO: Does this actually collect all of them?
  const DeclContext *DC = D->getDeclContext();
  while (DC) {
    DC = DC->getPrimaryContext();
    for (const Decl *Res : DC->decls())
      if (const auto *VD = dyn_cast<VarDecl>(Res))
        forAllFields(VD, FieldChainTy{}, [this, VD](const FieldChainTy &FC) {
          AllGlobalVariables.emplace(VD, FC);
        });
    DC = DC->getParent();
  }
}

void GenSetBuilder::getGenSetForCFGBlock(const CFGBlock *B, GenSet &Gen) {
  if (B->empty())
    return;

  VariableFinder.match(*D, D->getASTContext());

  CurrentGenSet = &Gen;

  for (CFGBlock::ConstCFGElementRef E : B->rrefs()) {
    if (E->getKind() != CFGElement::Kind::Statement)
      continue;
    CurrentCFGElem = E;

    const Stmt *S = E->castAs<CFGStmt>().getStmt();
    assert(S);
    DefinitionFinder.match(*S, D->getASTContext());
  }

  CurrentGenSet = nullptr;
}

void GenSetBuilder::getGenSetForParameters(const CFGBlock *Entry, GenSet &Gen) {
  CurrentGenSet = &Gen;
  CurrentCFGElem = {Entry, 0};
  if (const auto *F = dyn_cast<FunctionDecl>(D)) {
    for (const ParmVarDecl *Param : F->parameters()) {
      insertToGenSet(Param, Definition::DefinitionKind::Write);
    }
  }
  CurrentGenSet = nullptr;
}

void GenSetBuilder::handleExpr(const Expr *E, DefinitionKind Kind) {
  CurrentKind = Kind;
  ExpressionFinder.match(*E, *Context);
}

void GenSetBuilder::insertToGenSet(const VarDecl *Var, FieldChainTy FieldChain,
                                   DefinitionKind Kind) {
  forAllFields(Var, FieldChain, [this, Var, Kind](const FieldChainTy &FC) {
    CurrentGenSet->emplace(Var, FC, *CurrentCFGElem, Kind);
    // TODO: Invalidate all variables that can alias with this type.
  });
}

void GenSetBuilder::addVariable(const VarDecl *Var, FieldChainTy FieldChain) {
  forAllFields(Var, FieldChain, [this, Var](const FieldChainTy &FC) {
    AllVariables.emplace(Var, FC);
  });
}

//===----------------------------------------------------------------------===//
// Methods of ReachingDefinitionsCalculator.
//===----------------------------------------------------------------------===//

ReachingDefinitionsCalculator::ReachingDefinitionsCalculator(const Decl *D,
                                                             const CFG *cfg)
    : cfg(cfg), GSBuilder(D) {

  for (const CFGBlock *B : *cfg)
    GSBuilder.getGenSetForCFGBlock(B, Gen[B]);
  GSBuilder.getGenSetForParameters(&cfg->getEntry(), Gen[&cfg->getEntry()]);

  calculate();
}

void ReachingDefinitionsCalculator::init() {
  llvm::SmallVector<Definition, 16> AllDefinitions;
  for (const std::pair<const CFGBlock *, GenSet> G : Gen)
    for (const Definition &Def : G.second)
      AllDefinitions.push_back(Def);

  for (const std::pair<const CFGBlock *, GenSet> G : Gen)
    for (const Definition &Def : AllDefinitions)
      if (G.first != Def.getCFGBlock() && killsVar(Def, G.second))
        Kill[G.first].insert(Def);
}

using WorklistTy = llvm::SmallVector<const CFGBlock *, 5>;

void ReachingDefinitionsCalculator::calculate() {
  init();

  for (const std::pair<const CFGBlock *, GenSet> G : Gen)
    Out[G.first] = {G.second.begin(), G.second.end()};

  WorklistTy Worklist({cfg->begin(), cfg->end()});

  while (!Worklist.empty()) {
    const CFGBlock *N = Worklist.pop_back_val();

    for (const CFGBlock *Pred : N->preds())
      llvm::set_union(In[N], Out[Pred]);

    bool HasChanged =
        llvm::set_union(Out[N], llvm::set_difference(In[N], Kill[N]));

    if (llvm::set_union(Out[N], Gen[N]))
      HasChanged = true;

    if (HasChanged) {
      for (const CFGBlock *Succ : N->succs())
        Worklist.push_back(Succ);
    }
  }
}

void ReachingDefinitionsCalculator::dumpGenSet() const {
  llvm::errs() << "GEN sets: blockid (varname [blockid, elementid])\n";
  for (const std::pair<const CFGBlock *, GenSet> D : Gen) {
    size_t BlockID = llvm::find(*cfg, D.first) - cfg->begin();
    for (const Definition &Def : D.second) {
      llvm::errs() << BlockID << ' ';
      Def.dump();
      llvm::errs() << '\n';
    }
  }
}

void ReachingDefinitionsCalculator::dumpKillSet() const {
  llvm::errs() << "KILL sets: blockid (varname [blockid, elementid])\n";
  for (const std::pair<const CFGBlock *, DefinitionSet> D : Kill) {
    size_t BlockID = llvm::find(*cfg, D.first) - cfg->begin();
    for (const Definition &Def : D.second) {
      llvm::errs() << BlockID << ' ';
      Def.dump();
      llvm::errs() << '\n';
    }
  }
}

void ReachingDefinitionsCalculator::dumpReachingDefinitions() const {
  llvm::errs() << "Reaching definition sets: "
                  "blockid IN/OUT (varname [blockid, elementid])\n";
  for (const CFGBlock *B : *cfg) {
    size_t BlockID = llvm::find(*cfg, B) - cfg->begin();
    if (In.count(B)) {
      for (const Definition &Def : In.find(B)->second) {
        llvm::errs() << BlockID << " IN ";
        Def.dump();
        llvm::errs() << '\n';
      }
    }

    if (Out.count(B)) {
      for (const Definition &Def : Out.find(B)->second) {
        llvm::errs() << BlockID << " OUT ";
        Def.dump();
        llvm::errs() << '\n';
      }
    }
  }
}
