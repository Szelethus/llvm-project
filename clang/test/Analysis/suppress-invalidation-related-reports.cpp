// RUN: %clang_analyze_cc1 -verify=expected %s \
// RUN:   -analyzer-checker=core,apiModeling.SuppressInvalidationRelatedReports

// RUN: %clang_analyze_cc1 -verify=non-suppressed,expected %s \
// RUN:   -analyzer-checker=core

#include "Inputs/system-header-simulator.h"


//namespace invalidated_global {
//int flag;
//void foo();
//
//void f() {
//  flag = 0; // Initialize flag to false.
//  int *x = 0; // x is initialized to a nullptr.
//
//  foo(); // Invalidate flag's value.
//
//  if (flag) // Assume flag is true.
//    *x = 5; // non-suppressed-warning{{Dereference of null pointer}}
//}
//} // namespace invalidated_global
//
//namespace invalidated_field {
//struct S {
//  int flag;
//  void invalidate();
//};
//
//void f() {
//  S s;
//  s.flag = 0; // Initialize s.flag to false.
//  int *x = 0; // x is initialized to a nullptr.
//
//  s.invalidate(); // Invalidate s.flag's value.
//
//  if (s.flag) // Assume s.flag is true.
//    *x = 5; // non-suppressed-warning{{Dereference of null pointer}}
//}
//} // namespace invalidated_field
//
namespace invalidated_field_before_bind {
struct S {
  int flag;
  void invalidate();
  void setFlagToTrue() {
    invalidate();
    flag = 1;
  }
};

void f() {
  S s;
  s.flag = 0; // Initialize s.flag to false.
  int *x = 0; // x is initialized to a nullptr.

  s.setFlagToTrue(); // Invalidate s.flag's, but also set it to true.

  if (s.flag) // Assume s.flag is true.
    *x = 5; // expected-warning{{Dereference of null pointer}}
}
} // namespace invalidated_field_before_bind

//namespace invalidated_field_through_bind {
//struct S {
//  int flag;
//};
//
//void f() {
//  S s;
//  s.flag = 0; // Initialize s.flag to false.
//  S s2;
//  s2.flag = 1; // Initialize s2.flag to true
//  int *x = 0; // x is initialized to a nullptr.
//
//  s = s2; // "Invalidate" s.flag's value through bind.
//
//  if (s.flag) // Assume s.flag is true.
//    *x = 5; // expected-warning{{Dereference of null pointer}}
//}
//} // namespace invalidated_field_through_bind
