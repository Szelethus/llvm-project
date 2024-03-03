//===--- TaggedUnionMemberCountCheck.cpp - clang-tidy ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TaggedUnionMemberCountCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include "clang/AST/PrettyPrinter.h"
#include <algorithm>

using namespace clang::ast_matchers;

namespace clang::tidy::bugprone {

void TaggedUnionMemberCountCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(recordDecl(allOf(isStruct(), has(recordDecl(isUnion()).bind("union")), has(fieldDecl(hasType(enumDecl().bind("tags")))))), this);
}

void TaggedUnionMemberCountCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *unionMatch = Result.Nodes.getNodeAs<RecordDecl>("union");
  const auto *tagMatch  = Result.Nodes.getNodeAs<EnumDecl>("tags");
  int tagCount    = std::distance(tagMatch->enumerator_begin(), tagMatch->enumerator_end());
  int memberCount = std::distance(unionMatch->field_begin(), unionMatch->field_end());
  if (memberCount > tagCount) {
  	diag(unionMatch->getLocation(), "%0 has more data members than tags! Union member count: %1 Tag count: %2") << unionMatch << memberCount << tagCount;
  }
}

} // namespace clang::tidy::bugprone
