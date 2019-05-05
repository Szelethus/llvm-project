// RUN: %clang_analyze_cc1 -std=c++11 -Wno-null-dereference -verify %s \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=unix.Malloc \
// RUN:   -analyzer-checker=debug.AnalysisOrder \
// RUN:   -analyzer-config debug.AnalysisOrder:*=true

// expected-no-diagnostics

#include "Inputs/system-header-simulator-cxx.h"


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
