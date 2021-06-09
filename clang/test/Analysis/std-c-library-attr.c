// RUN: %clang_analyze_cc1 %s -verify \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=apiModeling.StdCLibraryFunctions \
// RUN:   -analyzer-checker=alpha.unix.StdCLibraryFunctionArgs \
// RUN:   -analyzer-checker=debug.StdCLibraryFunctionsTester

void foo(int i __attribute__((within_range(13, 99))),
    int j __attribute__((within_range(0, 10)))) {
   i = 12;
}

void k() {
  foo(5, 99); // \
  // expected-warning{{Function argument constraint is not satisfied}} \
  // expected-note{{The 1st arg should be within the range [13, 99]}}
}

void bar(int i __attribute__((out_of_range(13, 99)))) {
   i = 12;
}

void g() {
  bar(13); // \
  // expected-warning{{Function argument constraint is not satisfied}} \
  // expected-note{{The 1st arg should be out of the range [13, 99]}}
}
