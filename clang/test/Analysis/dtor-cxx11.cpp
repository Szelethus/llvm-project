// RUN: %clang_analyze_cc1 -std=c++11 -Wno-null-dereference -verify %s \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=unix.Malloc \
// RUN:   -analyzer-checker=debug.ExprInspection

// expected-no-diagnostics

#include "Inputs/system-header-simulator-cxx.h"

namespace Cxx11BraceInit {
struct Foo {
  ~Foo() {}
};

void testInitializerList() {
  for (Foo foo : {Foo(), Foo()}) {
  }
}
} // namespace Cxx11BraceInit

namespace pr41687 {
struct Foo {};
struct FooAndInt {
  FooAndInt(Foo, int) {}
};
void entry(int x) {
  FooAndInt(Foo(), ({
              return;
              x;
            }));
}
} // namespace pr41687
