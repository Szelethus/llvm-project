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

#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporterVisitors.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"

using namespace clang;
using namespace ento;

namespace {

class SuppressInvalidationRelatedReportsChecker
    : public Checker<check::RegionChanges> {
public:
  ProgramStateRef
  checkRegionChanges(ProgramStateRef State,
                     const InvalidatedSymbols *Invalidated,
                     ArrayRef<const MemRegion *> ExplicitRegions,
                     ArrayRef<const MemRegion *> Regions,
                     const LocationContext *LCtx, const CallEvent *Call) const {
    if (!Call)
      return State;

    for (const MemRegion *MR : State->get<HadInvalidation>()) {
      State = State->remove<HadInvalidation>(MR);
    }
    for (const MemRegion *MR : Regions) {
      //MR->dump();
      //llvm::errs() << '\n';
      State = State->add<HadInvalidation>(MR);
    }
    for (const MemRegion *MR : ExplicitRegions) {
      //MR->dump();
      //llvm::errs() << '\n';
      State = State->remove<HadInvalidation>(MR);
    }
    //printState(llvm::errs(), State, "\n", " ");
    //State->dump();
    //llvm::errs() << "CALLBACKED ==========\n";

    return State;
  }

  void printState(raw_ostream &Out, ProgramStateRef State,
                          const char *NL, const char *Sep) const override {
    Out << "Regions that went through invalidation at one point:" << NL;

    for (const MemRegion *MR : State->get<HadInvalidation>()) {
      MR->dumpToStream(Out);
      Out << NL;
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
