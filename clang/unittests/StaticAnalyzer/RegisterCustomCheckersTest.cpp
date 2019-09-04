//===- unittests/StaticAnalyzer/RegisterCustomCheckersTest.cpp ------------===//
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
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/AnalysisManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Frontend/AnalysisConsumer.h"
#include "clang/StaticAnalyzer/Frontend/CheckerRegistry.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"

namespace clang {
namespace ento {
namespace {

//===----------------------------------------------------------------------===//
// Just a minimal test for how checker registration works with statically
// linked, non TableGen generated checkers.
//===----------------------------------------------------------------------===//

class CustomChecker : public Checker<check::ASTCodeBody> {
public:
  void checkASTCodeBody(const Decl *D, AnalysisManager &Mgr,
                        BugReporter &BR) const {
    BR.EmitBasicReport(D, this, "Custom diagnostic", categories::LogicError,
                       "Custom diagnostic description",
                       PathDiagnosticLocation(D, Mgr.getSourceManager()), {});
  }
};

void addCustomChecker(AnalysisASTConsumer &AnalysisConsumer,
                      AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"custom.CustomChecker", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([](CheckerRegistry &Registry) {
    Registry.addChecker<CustomChecker>("custom.CustomChecker", "Description",
                                       "");
  });
}

TEST(RegisterCustomCheckers, RegisterChecker) {
  std::string Diags;
  EXPECT_TRUE(runCheckerOnCode<addCustomChecker>("void f() {;}", Diags));
  EXPECT_EQ(Diags, "custom.CustomChecker:Custom diagnostic description\n");
}

//===----------------------------------------------------------------------===//
// Pretty much the same.
//===----------------------------------------------------------------------===//

class LocIncDecChecker : public Checker<check::Location> {
public:
  void checkLocation(SVal Loc, bool IsLoad, const Stmt *S,
                     CheckerContext &C) const {
    auto UnaryOp = dyn_cast<UnaryOperator>(S);
    if (UnaryOp && !IsLoad) {
      EXPECT_FALSE(UnaryOp->isIncrementOp());
    }
  }
};

void addLocIncDecChecker(AnalysisASTConsumer &AnalysisConsumer,
                         AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"test.LocIncDecChecker", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([](CheckerRegistry &Registry) {
    Registry.addChecker<CustomChecker>("test.LocIncDecChecker", "Description",
                                       "");
  });
}

TEST(RegisterCustomCheckers, CheckLocationIncDec) {
  EXPECT_TRUE(
      runCheckerOnCode<addLocIncDecChecker>("void f() { int *p; (*p)++; }"));
}

//===----------------------------------------------------------------------===//
// Subchecker system.
//===----------------------------------------------------------------------===//

enum CXX23ModelingDiagKind { IntPointer, NonLoad };

class CXX23Modeling
    : public SuperChecker<CXX23ModelingDiagKind, check::ASTCodeBody> {
public:
  void checkASTCodeBody(const Decl *D, AnalysisManager &Mgr,
                        BugReporter &BR) const {
    BR.EmitBasicReport(D, this, "Custom diagnostic", categories::LogicError,
                       "Sketchy C++23 code modeled",
                       PathDiagnosticLocation(D, Mgr.getSourceManager()), {});

    if (const CheckerBase *IntPointerChecker = getSubChecker<IntPointer>())
      BR.EmitBasicReport(D, IntPointerChecker, "Custom diagnostic",
                         categories::LogicError, "Sketchy C++23 int pointer",
                         PathDiagnosticLocation(D, Mgr.getSourceManager()), {});

    if (const CheckerBase *NonLoadChecker = getSubChecker<NonLoad>())
      BR.EmitBasicReport(D, NonLoadChecker, "Custom diagnostic",
                         categories::LogicError,
                         "Sketchy C++23 pointer non-loaded",
                         PathDiagnosticLocation(D, Mgr.getSourceManager()), {});
  }
};

void registerCXX23IntPointer(CheckerManager &Mgr) {
  Mgr.registerSubChecker<CXX23Modeling, CXX23ModelingDiagKind::IntPointer>();
}

void registerCXX23NonLoad(CheckerManager &Mgr) {
  Mgr.registerSubChecker<CXX23Modeling, CXX23ModelingDiagKind::NonLoad>();
}

void addButDontSpecifyCXX23Modeling(AnalysisASTConsumer &AnalysisConsumer,
                      AnalyzerOptions &AnOpts) {
  AnalysisConsumer.AddCheckerRegistrationFn([](CheckerRegistry &Registry) {
    Registry.addChecker<CXX23Modeling>("test.CXX23Modeling", "Description", "");
  });
}

void addAndEnableCXX23Modeling(AnalysisASTConsumer &AnalysisConsumer,
                      AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"test.CXX23Modeling", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([](CheckerRegistry &Registry) {
    Registry.addChecker<CXX23Modeling>("test.CXX23Modeling", "Description", "");
  });
}

void addButDisableCXX23Modeling(AnalysisASTConsumer &AnalysisConsumer,
                      AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"test.CXX23Modeling", false}};
  AnalysisConsumer.AddCheckerRegistrationFn([](CheckerRegistry &Registry) {
    Registry.addChecker<CXX23Modeling>("test.CXX23Modeling", "Description", "");
  });
}

void addCXX23IntPointer(AnalysisASTConsumer &AnalysisConsumer,
                        AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages.emplace_back("test.CXX23IntPointer", true);
  AnalysisConsumer.AddCheckerRegistrationFn([](CheckerRegistry &Registry) {
    Registry.addChecker(registerCXX23IntPointer, CheckerRegistry::returnTrue,
                        "test.CXX23IntPointer", "Description", "",
                        /*IsHidden*/ false);
    Registry.addDependency("test.CXX23IntPointer", "test.CXX23Modeling");
  });
}

void addCXX23NonLoad(AnalysisASTConsumer &AnalysisConsumer,
                     AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages.emplace_back("test.CXX23NonLoad", true);
  AnalysisConsumer.AddCheckerRegistrationFn([](CheckerRegistry &Registry) {
    Registry.addChecker(registerCXX23NonLoad, CheckerRegistry::returnTrue,
                        "test.CXX23NonLoad", "Description", "",
                        /*IsHidden*/ false);
    Registry.addDependency("test.CXX23NonLoad", "test.CXX23Modeling");
  });
}

TEST(RegisterCustomCheckers, SuperChecker) {
  std::string Output;
  EXPECT_TRUE(runCheckerOnCode<addAndEnableCXX23Modeling>(
      "void foo(int *a) { *a; }", Output));
  EXPECT_EQ(Output, "test.CXX23Modeling:Sketchy C++23 code modeled\n");

  Output.clear();
  bool ReturnValue =
      runCheckerOnCode<addAndEnableCXX23Modeling, addCXX23IntPointer>(
          "void foo(int *a) { *a; }", Output);
  EXPECT_TRUE(ReturnValue);
  EXPECT_EQ(Output, "test.CXX23Modeling:Sketchy C++23 code modeled\n"
                    "test.CXX23IntPointer:Sketchy C++23 int pointer\n");

  Output.clear();
  ReturnValue =
      runCheckerOnCode<addAndEnableCXX23Modeling, addCXX23IntPointer,
                       addCXX23NonLoad>("void foo(int *a) { *a; }", Output);
  EXPECT_TRUE(ReturnValue);
  EXPECT_EQ(Output, "test.CXX23Modeling:Sketchy C++23 code modeled\n"
                    "test.CXX23IntPointer:Sketchy C++23 int pointer\n"
                    "test.CXX23NonLoad:Sketchy C++23 pointer non-loaded\n");

  Output.clear();
  ReturnValue =
      runCheckerOnCode<addButDontSpecifyCXX23Modeling, addCXX23IntPointer,
                       addCXX23NonLoad>("void foo(int *a) { *a; }", Output);
  EXPECT_TRUE(ReturnValue);
  EXPECT_EQ(Output, "test.CXX23Modeling:Sketchy C++23 code modeled\n"
                    "test.CXX23IntPointer:Sketchy C++23 int pointer\n"
                    "test.CXX23NonLoad:Sketchy C++23 pointer non-loaded\n");

  Output.clear();
  ReturnValue =
      runCheckerOnCode<addButDisableCXX23Modeling, addCXX23IntPointer,
                       addCXX23NonLoad>("void foo(int *a) { *a; }", Output);
  EXPECT_TRUE(ReturnValue);
  EXPECT_EQ(Output, "");
}

} // namespace
} // namespace ento
} // namespace clang
