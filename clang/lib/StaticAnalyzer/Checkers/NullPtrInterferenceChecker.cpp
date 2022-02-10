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
#include "clang/Analysis/AnalysisDeclContext.h"
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
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/SVals.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/SymExpr.h"
#include "llvm/Support/ErrorHandling.h"
#include <memory>

using namespace clang;
using namespace ento;

static SymbolRef getSymbolFromBinarySymExpr(const BinarySymExpr *BSE) {
  if (auto *SIE = dyn_cast<SymIntExpr>(BSE))
    return SIE->getLHS();

  if (auto *SIE = dyn_cast<IntSymExpr>(BSE))
    return SIE->getRHS();

  return nullptr;
}

REGISTER_MAP_WITH_PROGRAMSTATE(NonNullConstrainedPtrs, const SymbolRef,
                               const LocationContext *)

namespace {

class NullPtrInterferenceVisitor : public BugReporterVisitor {
  SymbolRef Sym;
  const LocationContext *LCtx;

public:
  NullPtrInterferenceVisitor(SymbolRef Sym, const LocationContext *LCtx)
      : Sym(Sym), LCtx(LCtx) {}

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    static int x = 0;
    ID.AddPointer(&x);
  }

  static void *getTag() {
    static int Tag = 0;
    return static_cast<void *>(&Tag);
  }

  PathDiagnosticPieceRef VisitNode(const ExplodedNode *, BugReporterContext &,
                                   PathSensitiveBugReport &) override {
    return nullptr;
  }
};

class NullPtrInterferenceChecker : public Checker<check::BranchCondition> {
  BugType BT;

public:
  NullPtrInterferenceChecker()
      : BT(this, "Null pointer already constrained nonnull",
           "Nullptr inference") {}

  void checkBranchCondition(const Stmt *Condition, CheckerContext &Ctx) const {
    auto Cond = Ctx.getSVal(Condition).getAs<DefinedOrUnknownSVal>();
    if (!Cond)
      return;
    SymbolRef Sym = Cond->getAsSymbol();
    if (!Sym)
      return;
    if (!Sym->getType()->isAnyPointerType())
      return;
    Sym->dump();

    if (Ctx.getState()->isNonNull(*Cond).isConstrainedTrue()) {
      const ExplodedNode *N = Ctx.generateNonFatalErrorNode(Ctx.getState());
      if (!N)
        return;
      std::unique_ptr<PathSensitiveBugReport> R(
          std::make_unique<PathSensitiveBugReport>(BT, BT.getDescription(), N));
      R->addVisitor<NullPtrInterferenceVisitor>(Sym, Ctx.getLocationContext());
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
