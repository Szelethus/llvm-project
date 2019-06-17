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

TEST(CFG, CFGElementRefTest) {
  const char *Code = R"(int test3() {
                          int x,y,z;

                          x = y = z = 1;
                          if (x > 0) {
                            while (x >= 0){
                              while (y >= x) {
                                x = x-1;
                                y = y/2;
                              }
                            }
                          }
                          z = y;

                          return 0;
                        })";

  BuildResult Result = BuildCFG(Code);
  CFG *cfg = Result.getCFG();
  //                           <- [B2] <-
  //                          /          \
  // [B8 (ENTRY)] -> [B7] -> [B6] ---> [B5] -> [B4] -> [B3]
  //                   \       |         \              /
  //                    \      |          <-------------
  //                     \      \
  //                      --------> [B1] -> [B0 (EXIT)]
  auto GetBlock = [cfg] (unsigned Index) -> CFGBlock * {
    assert(Index < cfg->size());
    return *(cfg->begin() + Index);
  };

  const CFGBlock *InnerMostLoopBody = GetBlock(4);
  // Sanity check.
  EXPECT_EQ(InnerMostLoopBody->size(), 2);

  CFGElementRef AssignmentToX = InnerMostLoopBody->getCFGElementRef(0);
  CFGElementRef AssignmentToY = InnerMostLoopBody->getCFGElementRef(1);

  EXPECT_TRUE(AssignmentToX.precedes(AssignmentToY));
  EXPECT_EQ((*AssignmentToX).getKind(), CFGElement::Statement);
}

} // namespace
} // namespace analysis
} // namespace clang
