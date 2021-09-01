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
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/BugReporter/CommonBugCategories.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/AnalysisManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ExplodedGraph.h"
#include "clang/StaticAnalyzer/Frontend/AnalysisConsumer.h"
#include "clang/StaticAnalyzer/Frontend/CheckerRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <memory>

namespace clang {
namespace ento {
namespace {

//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

class StatefulChecker : public Checker<check::PreCall> {
  mutable std::unique_ptr<BugType> BT;

public:
  void checkPreCall(const CallEvent &Call, CheckerContext &C) const {
    if (Call.isCalled(CallDescription{"error", 1})) {
      const ExplodedNode *N = C.generateErrorNode();
      if (!N)
        return;
      if (!BT)
        BT.reset(new BugType(this->getCheckerName(), "error()",
                             categories::SecurityError));
      auto R =
          std::make_unique<PathSensitiveBugReport>(*BT, "error() called", N);
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
    void g(int &i) {
      i = 5;
      i = 0;
    }
    void error(int &);
    void f() {
      int i;
      i = 5;
      error(i);
    }
  )",
                                                   Diags));
  EXPECT_EQ(Diags, "test.StatefulChecker: error() called\n");
}

} // namespace
} // namespace ento
} // namespace clang
