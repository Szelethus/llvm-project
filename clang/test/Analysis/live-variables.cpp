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

int test() { // B13 (ENTRY)

  int r = 0; // B12
  for (int x = 0; // B12

      x < 10; // B11 -> B10 B1

      x++) { // B2 -> B11

    // Liveness info is not computed correctly due to the following expression.
    // This happens due to CFG being special cased for short circuit operators.
    int *p = getPtr(); // B10
    if (p != 0 && // B10 -> B9 B6

        getBool() && // B9 -> B8 B6
        foo().m && // B8 -> B7 B6

        getBool() // B7 -> B6

        ) // B6 (entire condition) -> B5 B4 (B5 is a temporary dtor block)

        { // B4 (branch) -> B3 B2

      r = *p; // B3 -> B11
    }
  }
  return r; // B1
} // B0 (EXIT)

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
