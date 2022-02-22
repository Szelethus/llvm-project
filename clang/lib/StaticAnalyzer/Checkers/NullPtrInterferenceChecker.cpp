//===- NullPtrInterferenceChecker.cpp -------------------------- -*- C++ -*--=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This checker defines the attack surface for generic taint propagation.
//
// The taint information produced by it might be useful to other checkers. For
// example, checkers should report errors which involve tainted data more
// aggressively, even if the involved symbols are under constrained.
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

// static SymbolRef getSymbolFromBinarySymExpr(const BinarySymExpr *BSE) {
//   if (auto *SIE = dyn_cast<SymIntExpr>(BSE))
//     return SIE->getLHS();
//
//   if (auto *SIE = dyn_cast<IntSymExpr>(BSE))
//     return SIE->getRHS();
//
//   return nullptr;
// }

//class NullPtrInterferenceVisitor : public BugReporterVisitor {
//  const MemRegion *MR;
//  const LocationContext *OriginLCtx;
//  bool IsSatisfied = false;
//
//public:
//  NullPtrInterferenceVisitor(const MemRegion *MR, const LocationContext *LCtx)
//      : MR(MR), OriginLCtx(LCtx) {}
//
//  void Profile(llvm::FoldingSetNodeID &ID) const override {
//    static int x = 0;
//    ID.AddPointer(&x);
//  }
//
//  static void *getTag() {
//    static int Tag = 0;
//    return static_cast<void *>(&Tag);
//  }
//
//  PathDiagnosticPieceRef VisitNode(const ExplodedNode *N,
//                                   BugReporterContext &BC,
//                                   PathSensitiveBugReport &R) override {
//    if (IsSatisfied)
//      return nullptr;
//
//    if (auto CE = N->getLocationAs<CallEnter>())
//      if (CE->getLocationContext() == OriginLCtx)
//        IsSatisfied = true;
//
//    ProgramStateRef State = N->getState();
//    ProgramStateRef SuccState = N->getFirstSucc()->getState();
//
//    if (!isConstrainedNonNull(State, MR) &&
//        isConstrainedNonNull(SuccState, MR)) {
//      IsSatisfied = true;
//
//      if (OriginLCtx != N->getLocationContext()) {
//        R.markInvalid(getTag(), "Constrained in another LC");
//        return nullptr;
//      }
//      return std::make_shared<PathDiagnosticEventPiece>(
//          PathDiagnosticLocation(N->getNextStmtForDiagnostics(),
//                                 BC.getSourceManager(),
//                                 N->getLocationContext()),
//          "Pointer assumed non-null here");
//    }
//    return nullptr;
//  }
//};

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

static bool isNonNullConstraintTautological(const ExplodedNode *N,
                                            const MemRegion *MR,
                                            PathSensitiveBugReport &R) {
  const LocationContext *OriginLCtx = N->getLocationContext();
  assert(!N->getFirstSucc() &&
         "This should be a leaf of the ExplodedGraph (at this point in the "
         "analysis)!");

  assert(isConstrainedNonNull(N->getState(), MR) &&
         "This pointer should be non-null!");

  N = N->getFirstPred();

  // Look for the node where the constraint was imposed.
  while (N->getFirstPred() && !isNodeBeforeNonNullConstraint(N, MR)) {
    N = N->getFirstPred();
  }
  const ExplodedNode *NonNullConstrainedN = N;
  N = N->getFirstPred();
  if (!N)
    return false;
  assert(!isConstrainedNonNull(N->getState(), MR) &&
         "Failed to find the node that constrained MR!");

  // ...

  for (const ExplodedNode *Succ : N->succs()) {
    if (Succ == NonNullConstrainedN)
      continue;
    if (!Succ->isSink())
      return false;
  }
  if (OriginLCtx == N->getLocationContext()) {
    R.addNote("Pointer assumed non-null here",
              PathDiagnosticLocation(
                  N->getNextStmtForDiagnostics(),
                  N->getState()->getAnalysisManager().getSourceManager(),
                  N->getLocationContext()));
    return true;
  }

  return false;
}

namespace {

class NullPtrInterferenceChecker : public Checker<check::BranchCondition> {
  BugType BT;

public:
  NullPtrInterferenceChecker()
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
      const ExplodedNode *N = Ctx.generateNonFatalErrorNode(Ctx.getState());
      if (!N)
        return;
      std::unique_ptr<PathSensitiveBugReport> R(
          std::make_unique<PathSensitiveBugReport>(BT, BT.getDescription(), N));
      if (!isNonNullConstraintTautological(N, MR, *R))
        return;

      // R->addVisitor<NullPtrInterferenceVisitor>(MR,
      // Ctx.getLocationContext());
      assert(isa<Expr>(Condition));

      bugreporter::trackExpressionValue(N, cast<Expr>(Condition), *R);
      Ctx.emitReport(std::move(R));
    }
  }
};

} // namespace

void ento::registerNullPtrInterferenceChecker(CheckerManager &Mgr) {
  Mgr.registerChecker<NullPtrInterferenceChecker>();
}

bool ento::shouldRegisterNullPtrInterferenceChecker(const CheckerManager &mgr) {
  return true;
}
