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

#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallDescription.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
#include "llvm/Support/ErrorHandling.h"

using namespace clang;
using namespace ento;

static SymbolRef getSymbolFromBinarySymExpr(BinarySymExpr *BSE) {
  if (auto *SIE = dyn_cast<SymIntExpr>(BSE))
    return SIE->getLHS();

  if (auto *SIE = dyn_cast<IntSymExpr>(BSE))
    return SIE->getRHS();

  return nullptr;
}

namespace {

class NullPtrInterferenceChecker : public Checker<eval::Assume> {
public:
  ProgramStateRef evalAssume(ProgramStateRef State, SVal Cond,
                             bool Assumption) const {
    Cond.dump();
    llvm::errs() << '\n';
    auto *CondBSE = dyn_cast_or_null<BinarySymExpr>(Cond.getAsSymbol());
    if (!CondBSE)
      return State;
    assert(!Cond.getAsSymbol());
    return State;
  }
};

} // namespace

void ento::registerNullPtrInterferenceChecker(CheckerManager &Mgr) {
  Mgr.registerChecker<NullPtrInterferenceChecker>();
}

bool ento::shouldRegisterNullPtrInterferenceChecker(const CheckerManager &mgr) {
  return true;
}
