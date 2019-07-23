//===- unittests/Analysis/CFGTest.cpp - CFG tests -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CFGBuildResult.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Analysis/CFG.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

namespace clang {
namespace analysis {
namespace {

// Constructing a CFG for a range-based for over a dependent type fails (but
// should not crash).
TEST(CFG, RangeBasedForOverDependentType) {
  const char *Code = "class Foo;\n"
                     "template <typename T>\n"
                     "void f(const T &Range) {\n"
                     "  for (const Foo *TheFoo : Range) {\n"
                     "  }\n"
                     "}\n";
  EXPECT_EQ(BuildResult::SawFunctionBody, BuildCFG(Code).getStatus());
}

// Constructing a CFG containing a delete expression on a dependent type should
// not crash.
TEST(CFG, DeleteExpressionOnDependentType) {
  const char *Code = "template<class T>\n"
                     "void f(T t) {\n"
                     "  delete t;\n"
                     "}\n";
  EXPECT_EQ(BuildResult::BuiltCFG, BuildCFG(Code).getStatus());
}

// Constructing a CFG on a function template with a variable of incomplete type
// should not crash.
TEST(CFG, VariableOfIncompleteType) {
  const char *Code = "template<class T> void f() {\n"
                     "  class Undefined;\n"
                     "  Undefined u;\n"
                     "}\n";
  EXPECT_EQ(BuildResult::BuiltCFG, BuildCFG(Code).getStatus());
}

TEST(CFG, IsLinear) {
  auto expectLinear = [](bool IsLinear, const char *Code) {
    BuildResult B = BuildCFG(Code);
    EXPECT_EQ(BuildResult::BuiltCFG, B.getStatus());
    EXPECT_EQ(IsLinear, B.getCFG()->isLinear());
  };

  expectLinear(true,  "void foo() {}");
  expectLinear(true,  "void foo() { if (true) return; }");
  expectLinear(true,  "void foo() { if constexpr (false); }");
  expectLinear(false, "void foo(bool coin) { if (coin) return; }");
  expectLinear(false, "void foo() { for(;;); }");
  expectLinear(false, "void foo() { do {} while (true); }");
  expectLinear(true,  "void foo() { do {} while (false); }");
  expectLinear(true,  "void foo() { foo(); }"); // Recursion is not our problem.
}

TEST(CFG, ElementRefIterator) {
  const char *Code = R"(void f() {
                          int i;
                          int j;
                          i = 5;
                          i = 6;
                          j = 7;
                        })";
  
  BuildResult B = BuildCFG(Code);
  EXPECT_EQ(BuildResult::BuiltCFG, B.getStatus());
  CFG *Cfg = B.getCFG();
  // [B2 (ENTRY)]
  //   Succs (1): B1

  // [B1]
  //   1: int i;
  //   2: int j;
  //   3: i = 5
  //   4: i = 6
  //   5: j = 7
  //   Preds (1): B2
  //   Succs (1): B0

  // [B0 (EXIT)]
  //   Preds (1): B1
  CFGBlock *MainBlock = *(Cfg->begin() + 1);
  for (CFGBlock::CFGElementRef : MainBlock->refs())
    ;
}

} // namespace
} // namespace analysis
} // namespace clang
