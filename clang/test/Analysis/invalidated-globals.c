// RUN: %clang_analyze_cc1 -verify %s \
// RUN:   -analyzer-checker=core,apiModeling

#include "Inputs/system-header-simulator.h"

int flag;
void foo(); // Function body is unknown.

int *get();

void f() {
  flag = 1;
  int *x = 0; // x is initialized to a nullptr

  foo(); // Invalidate flag's value.

  if (flag) // Assume flag is false.
    x = get();

  foo(); // Invalidate flag's value.

  if (flag) // Assume flag is true.
    *x = 5; // x is dereferenced as a nullptr
}
