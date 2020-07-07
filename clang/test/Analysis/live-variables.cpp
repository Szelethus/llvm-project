// RUN: %clang_analyze_cc1 -verify %s \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=debug.DumpLiveVars \
// RUN: 2>&1 | FileCheck %s

// expected-no-diagnostics

namespace PR18159 {
class B {
public:
  bool m;
  ~B() {} // The destructor ensures that the binary logical operator below is wrapped in the ExprWithCleanups.
};
B foo();
int getBool();
int getBool2();
int *getPtr();

int test() {
  int r = 0;
  for (int x = 0; x < 10; x++) {
    // Liveness info is not computed correctly due to the following expression.
    // This happens due to CFG being special cased for short circuit operators.
    int *p = getPtr();
    if (p != 0 && getBool() && foo().m && getBool2()) {
      r = *p;
    }
  }
  return r;
}

//  [B13 (ENTRY)]
//    |
//    V
//  [B12]
// int r = 0
// int x = 0
//  ^    \                    [B1]
//  |     -----------------> return r -------> [B0 (EXIT)]
//  |              \
//  |               V
//  |              [B11]
// [B2]           x < 10
// x++              |
//  ^               V
//  |              [B10]
//  |             int *p = getPtr()
//  |       ----- p != 0
//  |      /        |
//  |     |         V
//  |     |        [B9]
//  |     | ----- getBool()
//  |     |/        |
//  |     |         V
//  |     |        [B8]
//  |     | ----- foo().m
//  |     |/        |
//  |     |         V
//  |     |        [B7]
//  |     | ----- getBool2()
//  |     |/        |
//  |     |         V
//  |     |        [B6]
//  |     | ----- (temp dtor of B8)
//  |     |/        |
//  |     |         V
//  |     |        [B5]
//  |     | ----- ~B()
//  |     |/
//  |     V
//  |\   [B4]
//  | - if (B7)
//  |     |
//  |     V
//   \   [B3]
//    - r = *p

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


// https://en.wikipedia.org/wiki/Live_variable_analysis, second example
namespace wiki_example {

int f(int a, int b, int c, int d) { // B4 (ENTRY)

  a = 3; // B3
  b = 5; // B3
  d = 4; // B3
  int x = 100; // B3
  if (a > b) { // B3

    c = a + b; // B2
    d = 2; // B2

  }
  c = 4; // B1
  return b * d + c; // B1
} // B0 (EXIT)

// CHECK: [ B0 (live variables at block exit) ]
// CHECK-EMPTY:
// CHECK-NEXT: [ B1 (live variables at block exit) ]
// CHECK-EMPTY:
// CHECK-NEXT: [ B2 (live variables at block exit) ]
// CHECK-NEXT:  b {{.*}}
// CHECK-NEXT:  d {{.*}}
// CHECK-EMPTY:
// CHECK-NEXT: [ B3 (live variables at block exit) ]
// CHECK-NEXT:  a {{.*}}
// CHECK-NEXT:  b {{.*}}
// CHECK-NEXT:  d {{.*}}
// CHECK-EMPTY:
// CHECK-NEXT: [ B4 (live variables at block exit) ]

} // namespace wiki_example

void clang_analyzer_eval(bool);

void test_lambda_refcapture() {
  int a = 6;
  [&](int &a) { a = 42; }(a);
  clang_analyzer_eval(a == 42); // expected-warning{{TRUE}}
}
