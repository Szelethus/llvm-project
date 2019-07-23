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
  Cfg->dump({}, true);
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

  // Non-reverse, non-const version
  long Index = 0;
  for (CFGBlock::CFGElementRef ElementRef : MainBlock->refs()) {
    EXPECT_EQ(ElementRef.getParent(), MainBlock);
    EXPECT_EQ(ElementRef.getIndexInBlock(), Index);
    EXPECT_TRUE(ElementRef->getAs<CFGStmt>());
    EXPECT_TRUE((*ElementRef).getAs<CFGStmt>());
    ++Index;
  }
  EXPECT_TRUE(*MainBlock->ref_begin() < *(MainBlock->ref_begin() + 1));
  EXPECT_TRUE(*MainBlock->ref_begin() == *MainBlock->ref_begin());
  EXPECT_FALSE(*MainBlock->ref_begin() != *MainBlock->ref_begin());

  Index = 0;
  for (CFGBlock::ref_iterator It = MainBlock->ref_begin();
       It != MainBlock->ref_end(); ++It) {
    EXPECT_EQ(It->getParent(), MainBlock);
    EXPECT_EQ(It->getIndexInBlock(), Index);
    EXPECT_TRUE((*It)->getAs<CFGStmt>());
    ++Index;
  }
  EXPECT_TRUE(MainBlock->ref_begin() < MainBlock->ref_begin() + 1);
  EXPECT_TRUE(MainBlock->ref_begin() == MainBlock->ref_begin());
  EXPECT_FALSE(MainBlock->ref_begin() != MainBlock->ref_begin());

  // Non-reverse, non-const version
  const CFGBlock *CMainBlock = MainBlock;
  Index = 0;
  for (CFGBlock::ConstCFGElementRef ElementRef : CMainBlock->refs()) {
    EXPECT_EQ(ElementRef.getParent(), CMainBlock);
    EXPECT_EQ(ElementRef.getIndexInBlock(), Index);
    EXPECT_TRUE(ElementRef->getAs<CFGStmt>());
    EXPECT_TRUE((*ElementRef).getAs<CFGStmt>());
    ++Index;
  }
  EXPECT_TRUE(*CMainBlock->ref_begin() < *(CMainBlock->ref_begin() + 1));
  EXPECT_TRUE(*CMainBlock->ref_begin() == *CMainBlock->ref_begin());
  EXPECT_FALSE(*CMainBlock->ref_begin() != *CMainBlock->ref_begin());

  Index = 0;
  for (CFGBlock::const_ref_iterator It = CMainBlock->ref_begin();
       It != CMainBlock->ref_end(); ++It) {
    EXPECT_EQ(It->getParent(), CMainBlock);
    EXPECT_EQ(It->getIndexInBlock(), Index);
    EXPECT_TRUE((*It)->getAs<CFGStmt>());
    ++Index;
  }
  EXPECT_TRUE(CMainBlock->ref_begin() < CMainBlock->ref_begin() + 1);
  EXPECT_TRUE(CMainBlock->ref_begin() == CMainBlock->ref_begin());
  EXPECT_FALSE(CMainBlock->ref_begin() != CMainBlock->ref_begin());

  // Reverse, non-const version
  Index = 4;
  for (CFGBlock::CFGElementRef ElementRef : MainBlock->rrefs()) {
    EXPECT_EQ(ElementRef.getParent(), MainBlock);
    EXPECT_EQ(ElementRef.getIndexInBlock(), Index);
    EXPECT_TRUE(ElementRef->getAs<CFGStmt>());
    EXPECT_TRUE((*ElementRef).getAs<CFGStmt>());
    --Index;
  }
  EXPECT_TRUE(*MainBlock->ref_begin() < *(MainBlock->ref_begin() + 1));
  EXPECT_TRUE(*MainBlock->ref_begin() == *MainBlock->ref_begin());
  EXPECT_FALSE(*MainBlock->ref_begin() != *MainBlock->ref_begin());

  Index = 4;
  for (CFGBlock::reverse_ref_iterator It = MainBlock->rref_begin();
       It != MainBlock->rref_end(); ++It) {
    EXPECT_EQ(It->getParent(), MainBlock);
    EXPECT_EQ(It->getIndexInBlock(), Index);
    EXPECT_TRUE((*It)->getAs<CFGStmt>());
    --Index;
  }
  EXPECT_TRUE(MainBlock->ref_begin() < MainBlock->ref_begin() + 1);
  EXPECT_TRUE(MainBlock->ref_begin() == MainBlock->ref_begin());
  EXPECT_FALSE(MainBlock->ref_begin() != MainBlock->ref_begin());

  // Reverse, const version
  Index = 4;
  for (CFGBlock::ConstCFGElementRef ElementRef : CMainBlock->rrefs()) {
    EXPECT_EQ(ElementRef.getParent(), CMainBlock);
    EXPECT_EQ(ElementRef.getIndexInBlock(), Index);
    EXPECT_TRUE(ElementRef->getAs<CFGStmt>());
    EXPECT_TRUE((*ElementRef).getAs<CFGStmt>());
    --Index;
  }
  EXPECT_TRUE(*CMainBlock->ref_begin() < *(CMainBlock->ref_begin() + 1));
  EXPECT_TRUE(*CMainBlock->ref_begin() == *CMainBlock->ref_begin());
  EXPECT_FALSE(*CMainBlock->ref_begin() != *CMainBlock->ref_begin());

  Index = 4;
  for (CFGBlock::const_reverse_ref_iterator It = CMainBlock->rref_begin();
       It != CMainBlock->rref_end(); ++It) {
    EXPECT_EQ(It->getParent(), CMainBlock);
    EXPECT_EQ(It->getIndexInBlock(), Index);
    EXPECT_TRUE((*It)->getAs<CFGStmt>());
    --Index;
  }
  EXPECT_TRUE(CMainBlock->ref_begin() < CMainBlock->ref_begin() + 1);
  EXPECT_TRUE(CMainBlock->ref_begin() == CMainBlock->ref_begin());
  EXPECT_FALSE(CMainBlock->ref_begin() != CMainBlock->ref_begin());
}

} // namespace
} // namespace analysis
} // namespace clang
