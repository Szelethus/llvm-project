//===--- TaggedUnionMemberCountCheck.cpp - clang-tidy ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TaggedUnionMemberCountCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/Support/Casting.h"
#include <iterator>
#include <limits>

#include <string.h>

using namespace clang::ast_matchers;

namespace clang::tidy::bugprone {

void TaggedUnionMemberCountCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      recordDecl(
          allOf(isStruct(),
                has(fieldDecl(hasType(recordDecl(isUnion()).bind("union")))),
                has(fieldDecl(hasType(enumDecl().bind("tags"))))))
          .bind("root"),
      this);
}

void TaggedUnionMemberCountCheck::check(
    const MatchFinder::MatchResult &Result) {
  auto *Root = Result.Nodes.getNodeAs<RecordDecl>("root");
  auto *UnionMatch = Result.Nodes.getNodeAs<RecordDecl>("union");
  auto *TagMatch = Result.Nodes.getNodeAs<EnumDecl>("tags");

  // Early exit if there are multiple enums or unions in the struct
  int64_t Tags = 0;
  int64_t Unions = 0;
  for (const FieldDecl *R : Root->fields()) {
    TypeSourceInfo *Info = R->getTypeSourceInfo();
    QualType Qualtype = Info->getType();
    const Type *Type = Qualtype.getTypePtr();
    if (Type->isUnionType())
      Unions += 1;
    else if (Type->isEnumeralType())
      Tags += 1;
    if (Tags > 1 || Unions > 1)
      return;
  }

  int64_t MaxTagValue = std::numeric_limits<int64_t>::min();
  int64_t MinTagValue = std::numeric_limits<int64_t>::max();
  int64_t CounterEnumConstantCount = 0;
  int64_t CounterEnumConstantValue = 0;
  int64_t FirstCounterEnumConstantIndex = 0;

  for (EnumDecl::enumerator_iterator It = TagMatch->enumerator_begin();
       It != TagMatch->enumerator_end(); It++) {
    EnumConstantDecl *R = *It;
    int64_t EnumValue = R->getInitVal().getExtValue();

    auto IsLikelyCountValue = [](EnumConstantDecl *R) -> bool {
      char countStr[] = "count";
      const char *EnumConstantIdentifier = R->getIdentifier()->getNameStart();
      return strlen(EnumConstantIdentifier) >= strlen(countStr) &&
             0 == strcmp(EnumConstantIdentifier +
                             R->getIdentifier()->getLength() - strlen(countStr),
                         countStr);
    };

    if (IsLikelyCountValue(R)) {
      CounterEnumConstantValue = EnumValue;
      if (CounterEnumConstantCount == 0) {
        FirstCounterEnumConstantIndex =
            std::distance(TagMatch->enumerator_begin(), It);
      }
      CounterEnumConstantCount += 1;
    }

    if (EnumValue > MaxTagValue)
      MaxTagValue = EnumValue;
    if (EnumValue < MinTagValue)
      MinTagValue = EnumValue;
  }

  int64_t EnumCountFinal = MaxTagValue - MinTagValue + 1;
  int64_t MemberCount =
      std::distance(UnionMatch->field_begin(), UnionMatch->field_end());
  int64_t EnumConstantsCount =
      std::distance(TagMatch->enumerator_begin(), TagMatch->enumerator_end());

  if (MemberCount > EnumCountFinal) {
    diag(Root->getLocation(), "Tagged union has more data members than tags! "
                              "Data members: %0 Tags: %1")
        << MemberCount << EnumCountFinal;
  } else if (MemberCount < EnumCountFinal) {
    diag(Root->getLocation(), "Tagged union has less data members than tags! "
                              "Data members: %0 Tags: %1")
        << MemberCount << EnumCountFinal;
  } else if (CounterEnumConstantCount == 1 &&
             FirstCounterEnumConstantIndex == EnumConstantsCount - 1 &&
             CounterEnumConstantValue == MaxTagValue) {
    diag(Root->getLocation(), "Tagged union has more data members than tags! "
                              "Data members: %0 Tags: %1")
        << MemberCount << EnumCountFinal - CounterEnumConstantCount;
  }
}

} // namespace clang::tidy::bugprone
