// RUN: %clang_analyze_cc1 -analyzer-checker=core,core.MainCall %s -verify

void g();

int main() {
  g();
}

void g() {
  main(); // expected-warning{{Call to main, which is undefined [core.MainCall]}}
}
