// RUN: %clang_analyze_cc1 -verify %s -analyzer-output=text\
// RUN:   -analyzer-checker=core,apiModeling \
// RUN:   -analyzer-checker=debug.ExprInspection \
// RUN:   -analyzer-checker=alpha.core.ReverseNull \
// RUN:   -analyzer-config apiModeling.StdCLibraryFunctions:ModelPOSIX=true

void clang_analyzer_warnIfReached();
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

long a64l(const char *str64);

void tp3_posix_nonnull_constraint(const char *p) {
  a64l(p);
  // expected-note@-1{{Pointer assumed non-null here}}
  // expected-note@-2{{Consider moving the condition here}}
  if (p)
    // expected-note@-1{{Pointer is unconditionally non-null here}}
    // expected-warning@-2{{Pointer is unconditionally non-null here [alpha.core.ReverseNull]}}
    return;
}

char* getenv (const char* name);

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
// True negative test cases.
//===----------------------------------------------------------------------===//

void tn1_nested(int *p) {
  *p = 5;
}

void tn1_constraint_in_nested_stackframe(int *p) {
  tn1_nested(p);
  if (p)
    return;
}
