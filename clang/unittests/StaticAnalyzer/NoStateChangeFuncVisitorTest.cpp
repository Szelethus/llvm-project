//===- unittests/StaticAnalyzer/NoStateChangeFuncVisitorTest.cpp ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CheckerRegistration.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporterVisitors.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/BugReporter/CommonBugCategories.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/AnalysisManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ExplodedGraph.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
#include "clang/StaticAnalyzer/Frontend/AnalysisConsumer.h"
#include "clang/StaticAnalyzer/Frontend/CheckerRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <memory>

//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

REGISTER_TRAIT_WITH_PROGRAMSTATE(ErrorPrevented, bool)

namespace clang {
namespace ento {
namespace {

class ErrorNotPreventedFuncVisitor : public NoStateChangeFuncVisitor {
public:
  ErrorNotPreventedFuncVisitor()
      : NoStateChangeFuncVisitor(bugreporter::TrackingKind::Thorough) {}

  virtual bool
  wasModifiedInFunction(const ExplodedNode *CallEnterN,
                        const ExplodedNode *CallExitEndN) override {
    return CallEnterN->getState()->get<ErrorPrevented>() !=
           CallExitEndN->getState()->get<ErrorPrevented>();
  }

  virtual PathDiagnosticPieceRef
  maybeEmitNoteForObjCSelf(PathSensitiveBugReport &R,
                           const ObjCMethodCall &Call,
                           const ExplodedNode *N) override {
    return nullptr;
  }

  virtual PathDiagnosticPieceRef
  maybeEmitNoteForCXXThis(PathSensitiveBugReport &R,
                          const CXXConstructorCall &Call,
                          const ExplodedNode *N) override {
    return nullptr;
  }

  virtual PathDiagnosticPieceRef
  maybeEmitNoteForParameters(PathSensitiveBugReport &R, const CallEvent &Call,
                             const ExplodedNode *N) override {
    PathDiagnosticLocation L = PathDiagnosticLocation::create(
        N->getLocation(),
        N->getState()->getStateManager().getContext().getSourceManager());
    return std::make_shared<PathDiagnosticEventPiece>(
        L, "Returning without prevening the error");
  }

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    static int Tag = 0;
    ID.AddPointer(&Tag);
  }
};

class StatefulChecker : public Checker<check::PreCall> {
  mutable std::unique_ptr<BugType> BT;

public:
  void checkPreCall(const CallEvent &Call, CheckerContext &C) const {
    if (Call.isCalled(CallDescription{"preventError", 0})) {
      C.addTransition(C.getState()->set<ErrorPrevented>(true));
      return;
    }

    if (Call.isCalled(CallDescription{"allowError", 0})) {
      C.addTransition(C.getState()->set<ErrorPrevented>(false));
      return;
    }

    if (Call.isCalled(CallDescription{"error", 0})) {
      const ExplodedNode *N = C.generateErrorNode();
      if (!N)
        return;
      if (!BT)
        BT.reset(new BugType(this->getCheckerName(), "error()",
                             categories::SecurityError));
      auto R =
          std::make_unique<PathSensitiveBugReport>(*BT, "error() called", N);
      R->addVisitor<ErrorNotPreventedFuncVisitor>();
      C.emitReport(std::move(R));
    }
  }
};

void addStatefulChecker(AnalysisASTConsumer &AnalysisConsumer,
                        AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"test.StatefulChecker", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([](CheckerRegistry &Registry) {
    Registry.addChecker<StatefulChecker>("test.StatefulChecker", "Description",
                                         "");
  });
}

TEST(NoStateChangeFuncVisitor, ThoroughFunctionAnalysis) {
  std::string Diags;
  EXPECT_TRUE(runCheckerOnCode<addStatefulChecker>(R"(
    void error();
    void preventError();
    void allowError();

    void g() {
      //preventError();
    }

    void f() {
      g();
      error();
    }
  )",
                                                   Diags));
  EXPECT_EQ(Diags, "test.StatefulChecker: error() called\n");
}

} // namespace
} // namespace ento
} // namespace clang
