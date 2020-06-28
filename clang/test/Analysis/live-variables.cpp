// RUN: %clang_analyze_cc1 -verify %s \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=debug.DumpLiveVars \
// RUN: 2>&1 | FileCheck %s
//
// expected-no-diagnostics

namespace PR18159 {
class B {
public:
  bool m;
  ~B() {} // The destructor ensures that the binary logical operator below is wrapped in the ExprWithCleanups.
};
B foo();
int getBool();
int *getPtr();
int test() {
  int r = 0;
  for (int x = 0; x < 10; x++) {
    int *p = getPtr();
    // Liveness info is not computed correctly due to the following expression.
    // This happens due to CFG being special cased for short circuit operators.
    if (p != 0 && getBool() && foo().m && getBool()) {
      r = *p; // no warning
    }
  }
  return r;
}

// CHECK: [ B0 (live variables at block exit) ]
// CHECK-EMPTY:
// CHECK: [ B1 (live variables at block exit) ]
// CHECK-EMPTY:
// CHECK-EMPTY:
// CHECK-NEXT: [ B0 (live variables at block exit) ]
// CHECK-EMPTY:
// CHECK-NEXT: [ B1 (live variables at block exit) ]
// CHECK-EMPTY:
// CHECK-NEXT: [ B2 (live variables at block exit) ]
// CHECK-NEXT:  r {{.*}}
// CHECK-NEXT:  x {{.*}}
// CHECK-EMPTY:
// CHECK-NEXT: [ B3 (live variables at block exit) ]
// CHECK-NEXT:  r {{.*}}
// CHECK-NEXT:  x {{.*}}
// CHECK-EMPTY:
// CHECK-NEXT: [ B4 (live variables at block exit) ]
// CHECK-NEXT:  r {{.*}}
// CHECK-NEXT:  x {{.*}}
// CHECK-NEXT:  p {{.*}}
// CHECK-EMPTY:
// CHECK-NEXT: [ B5 (live variables at block exit) ]
// CHECK-NEXT:  r {{.*}}
// CHECK-NEXT:  x {{.*}}
// CHECK-NEXT:  p {{.*}}
// CHECK-EMPTY:
// CHECK-NEXT: [ B6 (live variables at block exit) ]
// CHECK-NEXT:  r {{.*}}
// CHECK-NEXT:  x {{.*}}
// CHECK-NEXT:  p {{.*}}
// CHECK-EMPTY:
// CHECK-NEXT: [ B7 (live variables at block exit) ]
// CHECK-NEXT:  r {{.*}}
// CHECK-NEXT:  x {{.*}}
// CHECK-NEXT:  p {{.*}}
// CHECK-EMPTY:
// CHECK-NEXT: [ B8 (live variables at block exit) ]
// CHECK-NEXT:  r {{.*}}
// CHECK-NEXT:  x {{.*}}
// CHECK-NEXT:  p {{.*}}
// CHECK-EMPTY:
// CHECK-NEXT: [ B9 (live variables at block exit) ]
// CHECK-NEXT:  r {{.*}}
// CHECK-NEXT:  x {{.*}}
// CHECK-NEXT:  p {{.*}}
// CHECK-EMPTY:
// CHECK: [ B10 (live variables at block exit) ]
// CHECK-NEXT:  r {{.*}}
// CHECK-NEXT:  x {{.*}}
// CHECK-NEXT:  p {{.*}}
// CHECK-EMPTY:
// CHECK-NEXT: [ B11 (live variables at block exit) ]
// CHECK-NEXT:  r {{.*}}
// CHECK-NEXT:  x {{.*}}
// CHECK-EMPTY:
// CHECK-NEXT: [ B12 (live variables at block exit) ]
// CHECK-NEXT:  r {{.*}}
// CHECK-NEXT:  x {{.*}}
// CHECK-EMPTY:
// CHECK-NEXT: [ B13 (live variables at block exit) ]


} // namespace PR18159
