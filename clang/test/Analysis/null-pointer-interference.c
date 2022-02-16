// RUN: %clang_analyze_cc1 -verify %s -analyzer-output=text\
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=alpha.core.NullPtrInterference

int *get();
void top() {
  int *p = get(); // expected-note{{'p' initialized here}}
  int x = *p;
  // xpected-note@-1{{Pointer assumed non-null here}}
  if (p)
    // expected-note@-1{{Pointer already constrained nonnull}}
    // expected-warning@-2{{Pointer already constrained nonnull [alpha.core.NullPtrInterference]}}
    return;
}

void nested(int *p) {
  *p = 5;
}

void top2() {
  int *p = get();
  nested(p);
  if (p)
    return;
}
