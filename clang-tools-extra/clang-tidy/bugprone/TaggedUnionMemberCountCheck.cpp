//===--- TaggedUnionMemberCountCheck.cpp - clang-tidy ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TaggedUnionMemberCountCheck.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include "clang/AST/PrettyPrinter.h"
#include "llvm/Support/Casting.h"
#include <iterator>
#include <limits>

#include <string.h>

using namespace clang::ast_matchers;

namespace clang::tidy::bugprone {

void TaggedUnionMemberCountCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(recordDecl(allOf(isStruct(), has(fieldDecl(hasType(recordDecl(isUnion()).bind("union")))), has(fieldDecl(hasType(enumDecl().bind("tags")))))).bind("root"), this);
}

void TaggedUnionMemberCountCheck::check(const MatchFinder::MatchResult &Result) {
  auto *root       = Result.Nodes.getNodeAs<RecordDecl>("root");
  auto *unionMatch = Result.Nodes.getNodeAs<RecordDecl>("union");
  auto *tagMatch   = Result.Nodes.getNodeAs<EnumDecl>("tags");

  // Early exit if there are multiple enums or unions in the struct
  int64_t tags = 0;
  int64_t unions = 0;
  for (RecordDecl::field_iterator it = root->field_begin(); it != root->field_end(); it++) {
    const FieldDecl *r = *it;
    TypeSourceInfo *info = r->getTypeSourceInfo();
    QualType qualtype = info->getType();
    const Type *type = qualtype.getTypePtr();
    if (type->isUnionType()) unions += 1;
    else if (type->isEnumeralType()) tags += 1;
    if (tags > 1 || unions > 1) return;
  }

  int64_t maxTagValue = std::numeric_limits<int64_t>::min();
  int64_t minTagValue = std::numeric_limits<int64_t>::max();
  int64_t counterEnumConstantCount = 0;
  int64_t counterEnumConstantValue = 0;
  int64_t firstCounterEnumConstantIndex = 0;

  for (EnumDecl::enumerator_iterator it = tagMatch->enumerator_begin(); it != tagMatch->enumerator_end(); it++) {
    EnumConstantDecl *r = *it;
	int64_t enumValue = r->getInitVal().getExtValue();

    char countStr[] = "count";
	const char *enumConstantIdentifier = r->getIdentifier()->getNameStart();
	if (strlen(enumConstantIdentifier) >= strlen(countStr) && 0 == strcmp(enumConstantIdentifier + r->getIdentifier()->getLength() - strlen(countStr), countStr)) {
		counterEnumConstantValue = enumValue;
		if (counterEnumConstantCount == 0) {
			firstCounterEnumConstantIndex = std::distance(tagMatch->enumerator_begin(), it);
		}
		counterEnumConstantCount += 1;
	}

	if (enumValue > maxTagValue) maxTagValue = enumValue;
	if (enumValue < minTagValue) minTagValue = enumValue;
  }

  int64_t enumCountFinal = maxTagValue - minTagValue + 1;
  int64_t memberCount = std::distance(unionMatch->field_begin(), unionMatch->field_end());
  int64_t enumConstantsCount = std::distance(tagMatch->enumerator_begin(), tagMatch->enumerator_end());
  
  if (memberCount > enumCountFinal) {
    diag(root->getLocation(), "Tagged union has more data members than tags! Data members: %0 Tags: %1") << memberCount << enumCountFinal;
  }
  else if (memberCount < enumCountFinal) {
    diag(root->getLocation(), "Tagged union has less data members than tags! Data members: %0 Tags: %1") << memberCount << enumCountFinal;
  }
  else if (counterEnumConstantCount == 1 && firstCounterEnumConstantIndex == enumConstantsCount - 1 && counterEnumConstantValue == maxTagValue) {
    diag(root->getLocation(), "Tagged union has more data members than tags! Data members: %0 Tags: %1") << memberCount << enumCountFinal - counterEnumConstantCount;
  }

}

} // namespace clang::tidy::bugprone
