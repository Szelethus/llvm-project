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
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <memory>

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
    const auto *UnaryOp = dyn_cast<UnaryOperator>(S);
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
// Unsatisfied checker dependency
//===----------------------------------------------------------------------===//

class CheckerRegistrationOrderPrinter
    : public Checker<check::PreStmt<DeclStmt>> {
  std::unique_ptr<BuiltinBug> BT =
      std::make_unique<BuiltinBug>(this, "Registration order");

public:
  void checkPreStmt(const DeclStmt *DS, CheckerContext &C) const {
    ExplodedNode *N = nullptr;
    N = C.generateErrorNode();
    llvm::SmallString<200> Buf;
    llvm::raw_svector_ostream OS(Buf);
    C.getAnalysisManager()
        .getCheckerManager()
        ->getCheckerRegistry()
        .printEnabledCheckerList(OS);
    // Strip a newline off.
    auto R =
        std::make_unique<PathSensitiveBugReport>(*BT, OS.str().drop_back(1), N);
    C.emitReport(std::move(R));
  }
};

void registerCheckerRegistrationOrderPrinter(CheckerManager &mgr) {
  mgr.registerChecker<CheckerRegistrationOrderPrinter>();
}

bool shouldRegisterCheckerRegistrationOrderPrinter(const CheckerManager &mgr) {
  return true;
}

void addCheckerRegistrationOrderPrinter(CheckerRegistry &Registry) {
  Registry.addChecker(registerCheckerRegistrationOrderPrinter,
                      shouldRegisterCheckerRegistrationOrderPrinter,
                      "custom.RegistrationOrder", "Description", "", false);
}

#define UNITTEST_CHECKER(CHECKER_NAME, DIAG_MSG)                               \
  class CHECKER_NAME : public Checker<check::PreStmt<DeclStmt>> {              \
    std::unique_ptr<BuiltinBug> BT =                                           \
        std::make_unique<BuiltinBug>(this, DIAG_MSG);                          \
                                                                               \
  public:                                                                      \
    void checkPreStmt(const DeclStmt *DS, CheckerContext &C) const {}          \
  };                                                                           \
                                                                               \
  void register##CHECKER_NAME(CheckerManager &mgr) {                           \
    mgr.registerChecker<CHECKER_NAME>();                                       \
  }                                                                            \
                                                                               \
  bool shouldRegister##CHECKER_NAME(const CheckerManager &mgr) {               \
    return true;                                                               \
  }                                                                            \
  void add##CHECKER_NAME(CheckerRegistry &Registry) {                          \
    Registry.addChecker(register##CHECKER_NAME, shouldRegister##CHECKER_NAME,  \
                        "custom." #CHECKER_NAME, "Description", "", false);    \
  }

UNITTEST_CHECKER(Prerequisite, "Req")

bool shouldRegisterPrerequisiteFALSE(const CheckerManager &mgr) {
  return false;
}

UNITTEST_CHECKER(Dependent, "Dep")

void addDependent(AnalysisASTConsumer &AnalysisConsumer,
                  AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"custom.Dependent", true},
                                {"custom.RegistrationOrder", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([](CheckerRegistry &Registry) {
    Registry.addChecker(registerPrerequisite, shouldRegisterPrerequisiteFALSE,
                        "custom.Prerequisite", "Description", "", false);
    addDependent(Registry);
    addCheckerRegistrationOrderPrinter(Registry);
    Registry.addDependency("custom.Dependent", "custom.Prerequisite");
  });
}

TEST(RegisterDependents, UnsatisfiedDependency) {
  std::string Diags;
  EXPECT_TRUE(runCheckerOnCode<addDependent>("void f() {int i;}", Diags));
  EXPECT_EQ(Diags, "custom.RegistrationOrder:custom.RegistrationOrder\n");
}

//===----------------------------------------------------------------------===//
// Weak checker dependencies.
//===----------------------------------------------------------------------===//

void addWeakDepCheckerBothEnabled(AnalysisASTConsumer &AnalysisConsumer,
                                  AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"custom.Dependent", true},
                                {"custom.Prerequisite", true},
                                {"custom.RegistrationOrder", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([=](CheckerRegistry &Registry) {
    addPrerequisite(Registry);
    addDependent(Registry);
    addCheckerRegistrationOrderPrinter(Registry);
    Registry.addWeakDependency("custom.Dependent", "custom.Prerequisite");
  });
}

void addWeakDepCheckerBothEnabledSwitched(AnalysisASTConsumer &AnalysisConsumer,
                                          AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"custom.Dependent", true},
                                {"custom.Prerequisite", true},
                                {"custom.RegistrationOrder", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([=](CheckerRegistry &Registry) {
    addPrerequisite(Registry);
    addDependent(Registry);
    addCheckerRegistrationOrderPrinter(Registry);
    Registry.addWeakDependency("custom.Prerequisite", "custom.Dependent");
  });
}

void addWeakDepCheckerDepDisabled(AnalysisASTConsumer &AnalysisConsumer,
                                  AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"custom.Dependent", true},
                                {"custom.Prerequisite", false},
                                {"custom.RegistrationOrder", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([=](CheckerRegistry &Registry) {
    addPrerequisite(Registry);
    addDependent(Registry);
    addCheckerRegistrationOrderPrinter(Registry);
    Registry.addWeakDependency("custom.Dependent", "custom.Prerequisite");
  });
}

void addWeakDepCheckerDepUnspecified(AnalysisASTConsumer &AnalysisConsumer,
                                     AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"custom.Dependent", true},
                                {"custom.RegistrationOrder", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([=](CheckerRegistry &Registry) {
    addPrerequisite(Registry);
    addDependent(Registry);
    addCheckerRegistrationOrderPrinter(Registry);
    Registry.addWeakDependency("custom.Dependent", "custom.Prerequisite");
  });
}

UNITTEST_CHECKER(StrongDep, "Strong")
UNITTEST_CHECKER(WeakDep, "Weak")
UNITTEST_CHECKER(WeakDep2, "Weak2")
UNITTEST_CHECKER(Dep, "Dep")
UNITTEST_CHECKER(Dep2, "Dep2")

void addWeakDepHasWeakDep(
    AnalysisASTConsumer &AnalysisConsumer, AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"custom.Dep", true},
                                {"custom.WeakDep", true},
                                {"custom.WeakDep2", true},
                                {"custom.RegistrationOrder", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([=](CheckerRegistry &Registry) {
    addStrongDep(Registry);
    addWeakDep(Registry);
    addWeakDep2(Registry);
    addDep(Registry);
    addDep2(Registry);
    addCheckerRegistrationOrderPrinter(Registry);
    Registry.addWeakDependency("custom.Dep", "custom.WeakDep");
    Registry.addWeakDependency("custom.WeakDep", "custom.WeakDep2");
  });
}

void addWeakDepTransitivity(
    AnalysisASTConsumer &AnalysisConsumer, AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"custom.Dep", true},
                                {"custom.WeakDep", false},
                                {"custom.WeakDep2", true},
                                {"custom.RegistrationOrder", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([=](CheckerRegistry &Registry) {
    addStrongDep(Registry);
    addWeakDep(Registry);
    addWeakDep2(Registry);
    addDep(Registry);
    addDep2(Registry);
    addCheckerRegistrationOrderPrinter(Registry);
    Registry.addWeakDependency("custom.Dep", "custom.WeakDep");
    Registry.addWeakDependency("custom.WeakDep", "custom.WeakDep2");
  });
}

TEST(RegisterDependents, SimpleWeakDependency) {
  std::string Diags;
  EXPECT_TRUE(runCheckerOnCode<addWeakDepCheckerBothEnabled>(
      "void f() {int i;}", Diags));
  EXPECT_EQ(Diags, "custom.RegistrationOrder:custom.Prerequisite\ncustom."
                   "Dependent\ncustom.RegistrationOrder\n");
  Diags.clear();

  // Mind that AnalyzerOption listed the enabled checker list in the same order,
  // but the dependencies are switched.
  EXPECT_TRUE(runCheckerOnCode<addWeakDepCheckerBothEnabledSwitched>(
      "void f() {int i;}", Diags));
  EXPECT_EQ(Diags, "custom.RegistrationOrder:custom.Dependent\ncustom."
                   "Prerequisite\ncustom.RegistrationOrder\n");
  Diags.clear();

  // Weak dependencies dont prevent dependent checkers from being enabled.
  EXPECT_TRUE(runCheckerOnCode<addWeakDepCheckerDepDisabled>(
      "void f() {int i;}", Diags));
  EXPECT_EQ(
      Diags,
      "custom.RegistrationOrder:custom.Dependent\ncustom.RegistrationOrder\n");
  Diags.clear();

  // Nor will they be enabled just because a dependent checker is.
  EXPECT_TRUE(runCheckerOnCode<addWeakDepCheckerDepUnspecified>(
      "void f() {int i;}", Diags));
  EXPECT_EQ(
      Diags,
      "custom.RegistrationOrder:custom.Dependent\ncustom.RegistrationOrder\n");
  Diags.clear();

  EXPECT_TRUE(
      runCheckerOnCode<addWeakDepTransitivity>("void f() {int i;}", Diags));
  EXPECT_EQ(Diags, "custom.RegistrationOrder:custom.WeakDep2\ncustom."
                   "Dep\ncustom.RegistrationOrder\n");
  Diags.clear();

  EXPECT_TRUE(
      runCheckerOnCode<addWeakDepHasWeakDep>("void f() {int i;}", Diags));
  EXPECT_EQ(Diags, "custom.RegistrationOrder:custom.WeakDep2\ncustom."
                   "WeakDep\ncustom.Dep\ncustom.RegistrationOrder\n");
  Diags.clear();
}

//===----------------------------------------------------------------------===//
// Interaction of weak and regular checker dependencies.
//===----------------------------------------------------------------------===//

void addWeakDepHasStrongDep(AnalysisASTConsumer &AnalysisConsumer,
                            AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"custom.Dep", true},
                                {"custom.StrongDep", true},
                                {"custom.WeakDep", true},
                                {"custom.RegistrationOrder", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([=](CheckerRegistry &Registry) {
    addStrongDep(Registry);
    addWeakDep(Registry);
    addDep(Registry);
    addCheckerRegistrationOrderPrinter(Registry);
    Registry.addDependency("custom.WeakDep", "custom.StrongDep");
    Registry.addWeakDependency("custom.Dep", "custom.WeakDep");
  });
}

void addDisabledWeakDepHasStrongDep(AnalysisASTConsumer &AnalysisConsumer,
                                    AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"custom.Dep", true},
                                {"custom.StrongDep", true},
                                {"custom.WeakDep", false},
                                {"custom.RegistrationOrder", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([=](CheckerRegistry &Registry) {
    addStrongDep(Registry);
    addWeakDep(Registry);
    addDep(Registry);
    addCheckerRegistrationOrderPrinter(Registry);
    Registry.addDependency("custom.WeakDep", "custom.StrongDep");
    Registry.addWeakDependency("custom.Dep", "custom.WeakDep");
  });
}

void addDisabledWeakDepHasUnspecifiedStrongDep(
    AnalysisASTConsumer &AnalysisConsumer, AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"custom.Dep", true},
                                {"custom.WeakDep", false},
                                {"custom.RegistrationOrder", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([=](CheckerRegistry &Registry) {
    addStrongDep(Registry);
    addWeakDep(Registry);
    addDep(Registry);
    addCheckerRegistrationOrderPrinter(Registry);
    Registry.addDependency("custom.WeakDep", "custom.StrongDep");
    Registry.addWeakDependency("custom.Dep", "custom.WeakDep");
  });
}

void addWeakDepHasDisabledStrongDep(AnalysisASTConsumer &AnalysisConsumer,
                                    AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"custom.Dep", true},
                                {"custom.StrongDep", false},
                                {"custom.WeakDep", true},
                                {"custom.RegistrationOrder", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([=](CheckerRegistry &Registry) {
    addStrongDep(Registry);
    addWeakDep(Registry);
    addDep(Registry);
    addCheckerRegistrationOrderPrinter(Registry);
    Registry.addDependency("custom.WeakDep", "custom.StrongDep");
    Registry.addWeakDependency("custom.Dep", "custom.WeakDep");
  });
}

void addWeakDepHasUnspecifiedButLaterEnabledStrongDep(
    AnalysisASTConsumer &AnalysisConsumer, AnalyzerOptions &AnOpts) {
  AnOpts.CheckersAndPackages = {{"custom.Dep", true},
                                {"custom.Dep2", true},
                                {"custom.WeakDep", true},
                                {"custom.RegistrationOrder", true}};
  AnalysisConsumer.AddCheckerRegistrationFn([=](CheckerRegistry &Registry) {
    addStrongDep(Registry);
    addWeakDep(Registry);
    addDep(Registry);
    addDep2(Registry);
    addCheckerRegistrationOrderPrinter(Registry);
    Registry.addDependency("custom.WeakDep", "custom.StrongDep");
    Registry.addDependency("custom.Dep2", "custom.StrongDep");
    Registry.addWeakDependency("custom.Dep", "custom.WeakDep");
  });
}

TEST(RegisterDependents, DependencyInteraction) {
  std::string Diags;
  // Strong dependencies are always registered before weak dependencies.
  EXPECT_TRUE(
      runCheckerOnCode<addWeakDepHasStrongDep>("void f() {int i;}", Diags));
  EXPECT_EQ(Diags, "custom.RegistrationOrder:custom.StrongDep\ncustom."
                   "WeakDep\ncustom.Dep\ncustom.RegistrationOrder\n");
  Diags.clear();

  // If a weak dependency is disabled, the checker itself can still be enabled.
  EXPECT_TRUE(runCheckerOnCode<addDisabledWeakDepHasStrongDep>(
      "void f() {int i;}", Diags));
  EXPECT_EQ(Diags, "custom.RegistrationOrder:custom.Dep\ncustom."
                   "RegistrationOrder\ncustom.StrongDep\n");
  Diags.clear();

  // If a weak dependency is disabled, the checker itself can still be enabled,
  // but it shouldn't enable a strong unspecified dependency.
  EXPECT_TRUE(runCheckerOnCode<addDisabledWeakDepHasUnspecifiedStrongDep>(
      "void f() {int i;}", Diags));
  EXPECT_EQ(Diags,
            "custom.RegistrationOrder:custom.Dep\ncustom.RegistrationOrder\n");
  Diags.clear();

  // A strong dependency of a weak dependency is disabled, so neither of them
  // should be enabled.
  EXPECT_TRUE(runCheckerOnCode<addWeakDepHasDisabledStrongDep>(
      "void f() {int i;}", Diags));
  EXPECT_EQ(Diags,
            "custom.RegistrationOrder:custom.Dep\ncustom.RegistrationOrder\n");
  Diags.clear();

  EXPECT_TRUE(
      runCheckerOnCode<addWeakDepHasUnspecifiedButLaterEnabledStrongDep>(
          "void f() {int i;}", Diags));
  EXPECT_EQ(Diags,
            "custom.RegistrationOrder:custom.StrongDep\ncustom.WeakDep\ncustom."
            "Dep\ncustom.Dep2\ncustom.RegistrationOrder\n");
  Diags.clear();
}

} // namespace
} // namespace ento
} // namespace clang
