// RUN: %clang_analyze_cc1 -verify %s \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=debug.ExprInspection \
// RUN:   -analyzer-checker=alpha.core.NullPtrInterference


void clang_analyzer_warnIfReached();

int *get();
void top() {
  int *p = get();
  int x = *p;
  // some code, but still within this function.

  // p is in a condition!
  if (p) {
    clang_analyzer_warnIfReached();
    return;
  }
  clang_analyzer_warnIfReached();
}

void nested(int *p) {
  *p = 5;
}

void top2() {
  int *p = get();
  nested(p);
  // some code, but still within this function.

  // p is in a condition!
  if (p) {
    clang_analyzer_warnIfReached();
    return;
  }
  clang_analyzer_warnIfReached();
}
