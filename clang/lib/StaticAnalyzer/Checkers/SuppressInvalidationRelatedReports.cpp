//=== FIXME.cpp -------------------------------------------*- C++ -*--//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  TODO
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ExprCXX.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporterVisitors.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/SymExpr.h"
#include "llvm/ADT/STLExtras.h"

using namespace clang;
using namespace ento;

namespace {

class SuppressInvalidationRelatedReportsChecker
    : public Checker<check::RegionChanges, check::PostStmt<CXXMemberCallExpr>> {
public:
  ProgramStateRef
  checkRegionChanges(ProgramStateRef State,
                     const InvalidatedSymbols *Invalidated,
                     ArrayRef<const MemRegion *> ExplicitRegions,
                     ArrayRef<const MemRegion *> Regions,
                     const LocationContext *LCtx, const CallEvent *Call) const {
    //State->dump();
    //llvm::errs() << '\n';
    //llvm::errs() << '\n';
    //LCtx->dump();
    //llvm::errs() << '\n';
    //llvm::errs() << '\n';
    //llvm::errs() << '\n';
      // add explicit binds and don't filetr no nothing
    if (!Call)
      return State;
    
    llvm::errs() << "NONNULL\n";

    for (const MemRegion *MR : Regions)
      State = State->set<HadInvalidation>(MR, Call->getLocationContext());

    return State;
  }

  void checkPostStmt(const CXXMemberCallExpr *M, CheckerContext &C) const {
    //M->dump();
    //assert(M->getMethodDecl()->hasBody());
  }

  void printState(raw_ostream &Out, ProgramStateRef State, const char *NL,
                  const char *Sep) const override {
    for (const HadInvalidation::value_type &I : State->get<HadInvalidation>()) {
      I.first->dumpToStream(Out);
      Out << " was invalidated by '";
      if (const auto *ND = dyn_cast<NamedDecl>(I.second->getDecl()))
        if (const IdentifierInfo *II = ND->getIdentifier())
          Out << II->getName();
      Out << "'" << NL;
    }
  }
};

} // end anonymous namespace

void ento::registerSuppressInvalidationRelatedReportsChecker(
    CheckerManager &mgr) {
  mgr.registerChecker<SuppressInvalidationRelatedReportsChecker>();
}

bool ento::shouldRegisterSuppressInvalidationRelatedReportsChecker(
    const LangOptions &LO) {
  return true;
}
