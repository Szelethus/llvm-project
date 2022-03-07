// RUN: %clang_analyze_cc1 -verify %s -analyzer-output=text\
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=apiModeling \
// RUN:   -analyzer-checker=debug.ExprInspection \
// RUN:   -analyzer-checker=alpha.core.ReverseNull \
// RUN:   -analyzer-config apiModeling.StdCLibraryFunctions:ModelPOSIX=true \
// RUN:   -analyzer-config apiModeling.StdCLibraryFunctions:DisplayLoadedSummaries=true

// RUN: %clang_analyze_cc1 -verify %s -analyzer-output=text\
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=apiModeling \
// RUN:   -analyzer-checker=debug.ExprInspection \
// RUN:   -analyzer-checker=alpha.core.ReverseNull \
// RUN:   -analyzer-config apiModeling.StdCLibraryFunctions:ModelPOSIX=true \
// RUN:   -analyzer-config apiModeling.StdCLibraryFunctions:DisplayLoadedSummaries=true \
// RUN:   2>&1 | FileCheck %s

void clang_analyzer_warnIfReached();
int coin();
//===----------------------------------------------------------------------===//
// True positive test cases.
//===----------------------------------------------------------------------===//

void tp1(int *p) {
  *p = 5;
  // expected-note@-1{{Pointer assumed non-null here}}
  // expected-note@-2{{Consider moving the condition here}}
  if (p)
    // expected-note@-1{{Pointer is unconditionally non-null here}}
    // expected-warning@-2{{Pointer is unconditionally non-null here [alpha.core.ReverseNull]}}
    return;
}

//===----------------------------------------------------------------------===//

void tp2(int *p) {
  *p = 5;
  // expected-note@-1{{Pointer assumed non-null here}}
  // expected-note@-2{{Consider moving the condition here}}
  int *x = p;
  // expected-note@-1{{'x' initialized here}}
  if (x)
    // expected-note@-1{{Pointer is unconditionally non-null here}}
    // expected-warning@-2{{Pointer is unconditionally non-null here [alpha.core.ReverseNull]}}
    return;
}

//===----------------------------------------------------------------------===//

char *getenv(const char *name);
long a64l(const char *str64);
// CHECK: Loaded summary for: char *getenv(const char *name)
// CHECK: Loaded summary for: long a64l(const char *str64)

void tp3_posix_nonnull_constraint(const char *p) {
  a64l(p);
  // expected-note@-1{{Pointer assumed non-null here}}
  // expected-note@-2{{Consider moving the condition here}}
  if (p)
    // expected-note@-1{{Pointer is unconditionally non-null here}}
    // expected-warning@-2{{Pointer is unconditionally non-null here [alpha.core.ReverseNull]}}
    return;
}

//===----------------------------------------------------------------------===//

void tp4_nonnull_constraint(const char *p) {
  getenv(p);
  // expected-note@-1{{Pointer assumed non-null here}}
  // expected-note@-2{{Consider moving the condition here}}
  if (p)
    // expected-note@-1{{Pointer is unconditionally non-null here}}
    // expected-warning@-2{{Pointer is unconditionally non-null here [alpha.core.ReverseNull]}}
    return;
}

//===----------------------------------------------------------------------===//

void tp_5_constraint_does_not_dominate_condition_but_condition_postdominates_constraint(int *p) {
  if (coin()) {
    // expected-note@-1{{Assuming the condition is true}}
    // expected-note@-2{{Taking true branch}}
    *p = 5;
    // expected-note@-1{{Pointer assumed non-null here}}
    // expected-note@-2{{Consider moving the condition here}}
  }
  if (p)
    // expected-note@-1{{Pointer is non-null here}}
    // expected-warning@-2{{Pointer is non-null here [alpha.core.ReverseNull]}}
    return;
}

//===----------------------------------------------------------------------===//

void tp_7_constraint_dominates_condition(int *p) {
  *p = 5;
  // expected-note@-1{{Pointer assumed non-null here}}
  // expected-note@-2{{Consider moving the condition here}}
  if (coin()) {
    // expected-note@-1{{Assuming the condition is true}}
    // expected-note@-2{{Taking true branch}}
    if (p)
      // expected-note@-1{{Pointer is unconditionally non-null here}}
      // expected-warning@-2{{Pointer is unconditionally non-null here [alpha.core.ReverseNull]}}
      return;
  }
}

//===----------------------------------------------------------------------===//
// True negative test cases.
//===----------------------------------------------------------------------===//

void tn1_constrain_arg(int *p) {
  *p = 5;
}

void tn1_constraint_in_nested_stackframe(int *p) {
  tn1_constrain_arg(p);
  if (p)
    return;
}

//===----------------------------------------------------------------------===//

void tn2_constraint_in_parent_stackframe(int *p) {
  if (p)
    return;
}

void tn2_caller(int *p) {
  *p = 5;
  tn2_constraint_in_parent_stackframe(p);
}

//===----------------------------------------------------------------------===//

// Say this 3 times fast.
void tn3_constraint_does_not_dominate_condition_and_condition_doesnt_postdominate_constraint(int *p) {
  if (coin()) {
    *p = 5;
  }
  if (coin()) {
    if (p)
      return;
  }
}

//===----------------------------------------------------------------------===//

void tn4_constraint_point_before_dereference(int *p) {
  if (!p)
    return;
  *p = 5;
  if (p)
    return;
}
