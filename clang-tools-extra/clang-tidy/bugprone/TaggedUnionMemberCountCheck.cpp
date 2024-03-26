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
#include "llvm/Support/Casting.h"
#include <iterator>

using namespace clang::ast_matchers;

namespace clang::tidy::bugprone {

void TaggedUnionMemberCountCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(recordDecl(allOf(isStruct(), has(fieldDecl(hasType(recordDecl(isUnion()).bind("union")))), has(fieldDecl(hasType(enumDecl().bind("tags")))))).bind("root"), this);
}

void TaggedUnionMemberCountCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *root = Result.Nodes.getNodeAs<RecordDecl>("root");
  int tags = 0;
  int unions = 0;
  for (RecordDecl::field_iterator it = root->field_begin(); it != root->field_end(); it++) {
    if (auto *r = llvm::dyn_cast<FieldDecl>(*it)) {
      TypeSourceInfo *info = r->getTypeSourceInfo();
      QualType qualtype = info->getType();
      const Type *type = qualtype.getTypePtr();
      if (type->isUnionType()) unions += 1;
      else if (type->isEnumeralType()) tags += 1;
      if (tags > 1 || unions > 1) return;
    }
  }
  const auto *unionMatch = Result.Nodes.getNodeAs<RecordDecl>("union");
  const auto *tagMatch  = Result.Nodes.getNodeAs<EnumDecl>("tags");
  const int tagCount    = std::distance(tagMatch->enumerator_begin(), tagMatch->enumerator_end());
  const int memberCount = std::distance(unionMatch->field_begin(), unionMatch->field_end());
  if (memberCount > tagCount) {
    diag(root->getLocation(), "Tagged union has more data members than tags! Data members: %0 Tags: %1") << memberCount << tagCount;
  }
}

} // namespace clang::tidy::bugprone
