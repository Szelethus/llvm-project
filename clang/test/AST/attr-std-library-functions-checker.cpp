// RUN: %clang_cc1 %s -verify

void baz(int *ptr __attribute__((out_of_range(0, 0)))) { // \
  // expected-error{{out of range on non-integral parameter}}
}
