// MoveChecker.cpp - Check use of moved-from objects. - C++ ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This defines checker which checks for potential misuses of a moved-from
// object. That means method calls on the object or copying it in moved-from
// state.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Attr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallDescription.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "llvm/ADT/StringSet.h"

using namespace clang;
using namespace ento;

class MainChecker : public Checker<check::PreCall> {
  CallDescription MainFn{"main"};
  BugType MainCallBugType{this, "Call to main", "Callmain"};

public:
  void checkPreCall(const CallEvent &Call, CheckerContext &C) const {
    if (!MainFn.matches(Call))
      return;

    ExplodedNode *ErrNode = C.generateErrorNode();
    if (!ErrNode)
      return;
    llvm::SmallString<128> Str;
    llvm::raw_svector_ostream OS(Str);
    OS << "Call to main, which is undefined";
    auto R = std::make_unique<PathSensitiveBugReport>(MainCallBugType, OS.str(),
                                                      ErrNode);
    C.emitReport(std::move(R));
  }
};

bool clang::ento::shouldRegisterMainCallChecker(
    clang::ento::CheckerManager const &mgr) {
  return true;
}

void clang::ento::registerMainCallChecker(clang::ento::CheckerManager &mgr) {
  mgr.registerChecker<MainChecker>();
}
