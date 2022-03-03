// RUN: %clang_analyze_cc1 -verify %s -analyzer-output=text\
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=alpha.core.ReverseNull

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

//===----------------------------------------------------------------------===//
// True negative test cases.
//===----------------------------------------------------------------------===//

//void tn1_nested(int *p) {
//  *p = 5;
//}
//
//void tn1(int *p) {
//  tn1_nested(p);
//  if (p)
//    return;
//}
