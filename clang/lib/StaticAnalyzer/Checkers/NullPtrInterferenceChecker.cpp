//===--- ReverseNullChecker.cpp ----------------------------- -*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// TODO
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Stmt.h"
#include "clang/AST/StmtObjC.h"
#include "clang/Analysis/AnalysisDeclContext.h"
#include "clang/Analysis/PathDiagnostic.h"
#include "clang/Analysis/ProgramPoint.h"
#include "clang/Basic/AttrKinds.h"
#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporterVisitors.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallDescription.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ExplodedGraph.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/MemRegion.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramState.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramState_Fwd.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/SVals.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/SymExpr.h"
#include "llvm/Support/ErrorHandling.h"
#include <memory>

using namespace clang;
using namespace ento;

REGISTER_MAP_WITH_PROGRAMSTATE(NonNullConstrainedPtrs, const SymbolRef,
                               const LocationContext *)

static bool isConstrainedNonNull(ProgramStateRef State, const MemRegion *MR) {
  return State->isNonNull(loc::MemRegionVal(MR)).isConstrainedTrue();
}

static bool isNodeBeforeNonNullConstraint(const ExplodedNode *N,
                                          const MemRegion *MR) {
  assert(N);
  assert(N->getFirstPred());
  return !isConstrainedNonNull(N->getFirstPred()->getState(), MR) &&
         isConstrainedNonNull(N->getState(), MR);
}

/// Leave a note about where the constraint to null/non-null was imposed.
class ReverseNullVisitor : public BugReporterVisitor {
  /// The memory region that was a part of a condition where its value was
  /// already known.
  const MemRegion *MR;
  bool IsSatisfied = false;

public:
  ReverseNullVisitor(const MemRegion *MR) : MR(MR) {}

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    static int x = 0;
    ID.AddPointer(&x);
  }

  static void *getTag() {
    static int Tag = 0;
    return static_cast<void *>(&Tag);
  }

  PathDiagnosticPieceRef VisitNode(const ExplodedNode *N,
                                   BugReporterContext &BC,
                                   PathSensitiveBugReport &R) override {
    if (IsSatisfied)
      return nullptr;

    if (!isNodeBeforeNonNullConstraint(N, MR))
      return nullptr;

    IsSatisfied = true;

    return std::make_shared<PathDiagnosticEventPiece>(
        PathDiagnosticLocation(N->getNextStmtForDiagnostics(),
                               BC.getSourceManager(), N->getLocationContext()),
        "Pointer assumed non-null here");
  }

  /// The checker must have made sure that this node actually exists. If we did
  /// not find it, we must've messed up the visitor. Are we sure that we used
  /// the same machinery to find the exploded here AND in the checker?
  virtual void finalizeVisitor(BugReporterContext &BRC,
                               const ExplodedNode *EndPathNode,
                               PathSensitiveBugReport &BR) override {
    assert(IsSatisfied &&
           "Failed to find the ExplodedNode where the constaint was imposed on "
           "the condition whose value was known!");
  }
};

/// Climb up the ExplodedGraph, and check whether the point where the condition
/// was contrained to be null/non-null was a state split to analyze two new
/// paths of execution, or only one.
/// For example, here, p is null on one path, and non-null on another where the
/// constraints are imposed:
///
///   int *p = get();
///   if (p) // state split, this ExplodedNode will have two children
///     // p assumed non-null
///   else
///     // p assumed non-null
///   //...
///   if (p) // Don't warn here, this is a totally valid condition
///     // ...
///
/// But here, on all paths immediately after p's dereference, p is non-null:
///
///   int *p = get();
///   *p = 5; // This ExplodedNode will have a single child
///   if (p) // Warn here!
///     // ...
///
/// Ideally, this would be done with a BugReporterVisitor, but the ExplodedGraph
/// it has access to is NOT the same as the ExplodedGraph that is constructed
/// during analysis (see https://reviews.llvm.org/D65379). In fact, it is a
/// linear graph, and we're specifically interested in branches, hence the
/// unusual approach.
static bool isNonNullConstraintTautological(const ExplodedNode *N,
                                            const MemRegion *MR) {

  // The location context of the condition point.
  const LocationContext *OriginLCtx = N->getLocationContext();
  assert(!N->getFirstSucc() &&
         "This should be a leaf of the ExplodedGraph (at this point in the "
         "analysis)!");

  assert(isConstrainedNonNull(N->getState(), MR) &&
         "This pointer should be non-null!");

  N = N->getFirstPred();

  // Look for the node where the constraint was imposed.
  while (N->getFirstPred() && !isNodeBeforeNonNullConstraint(N, MR))
    N = N->getFirstPred();

  // We failed to find the node. This can happen with ObjC self, which may not
  // have a node where its non-null, even if it can be non-null.
  if (!N)
    return false;

  const ExplodedNode *NonNullConstrainedN = N;
  const ExplodedNode *BeforeNonNullConstraintN = N->getFirstPred();

  assert(!isConstrainedNonNull(BeforeNonNullConstraintN->getState(), MR) &&
         "This is supposed to be the node where the constraint is not yet "
         "imposed!");

  // How many children does the pre-constraint node have? Does it have any that
  // does not unconditionally lead to a sink node?
  for (const ExplodedNode *Succ : BeforeNonNullConstraintN->succs()) {
    if (Succ == NonNullConstrainedN)
      continue;

    // Yes, it does. The condition was likely fine, or not necesserily a sign
    // of code smell.
    if (!Succ->isSink())
      return false;
  }

  // We want to be sure that the constraint and the condition are in the same
  // stackframe. Inlined functions' pre/post conditions may not apply to the
  // caller stackframe. A similar issue is discussed here:
  // https://discourse.llvm.org/t/static-analyzer-query-why-is-suppress-null-return-paths-enabled-by-default/
  return OriginLCtx == BeforeNonNullConstraintN->getLocationContext();
}

namespace {

class ReverseNullChecker : public Checker<check::BranchCondition> {
  BugType BT;

public:
  ReverseNullChecker()
      : BT(this, "Pointer already constrained nonnull", "Nullptr inference") {}

  void checkBranchCondition(const Stmt *Condition, CheckerContext &Ctx) const {
    if (isa<ObjCForCollectionStmt>(Condition))
      return;

    auto Cond = Ctx.getSVal(Condition).getAs<DefinedOrUnknownSVal>();
    if (!Cond)
      return;

    const MemRegion *MR = Cond->getAsRegion();
    if (!MR)
      return;

    if (isConstrainedNonNull(Ctx.getState(), MR)) {
      if (!isNonNullConstraintTautological(Ctx.getPredecessor(), MR))
        return;

      const ExplodedNode *N = Ctx.generateNonFatalErrorNode(Ctx.getState());
      if (!N)
        return;

      std::unique_ptr<PathSensitiveBugReport> R(
          std::make_unique<PathSensitiveBugReport>(BT, BT.getDescription(), N));

      R->addVisitor<ReverseNullVisitor>(MR);

      bugreporter::trackExpressionValue(N, cast<Expr>(Condition), *R);
      Ctx.emitReport(std::move(R));
    }
  }
};

} // namespace

void clang::ento::registerReverseNullChecker(CheckerManager &Mgr) {
  Mgr.registerChecker<ReverseNullChecker>();
}

bool clang::ento::shouldRegisterReverseNullChecker(const CheckerManager &mgr) {
  return true;
}
