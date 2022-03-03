// RUN: %clang_analyze_cc1 -verify %s -analyzer-output=text\
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=alpha.core.ReverseNull

//===----------------------------------------------------------------------===//
// True positive test cases.
//===----------------------------------------------------------------------===//

void tp1(int *p) {
  // expected-note@-1{{Pointer assumed non-null here}}
  if (p)
    // expected-note@-1{{Pointer already constrained nonnull}}
    // expected-warning@-2{{Pointer is unconditionally non-null here [alpha.core.ReverseNull]}}
    return;
}

//===----------------------------------------------------------------------===//
// True negative test cases.
//===----------------------------------------------------------------------===//

void tn1_nested(int *p) {
  *p = 5;
}

void tn1(int *p) {
  tn1_nested(p);
  if (p)
    return;
}
