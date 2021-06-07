
void foo(int i __attribute__((within_range(13, 99)))) {
   i = 12;  // warning
}

void k() {
  foo(0);
}
