// RUN: %clang_analyze_cc1 -analyzer-checker=core,debug.ExprInspection -verify %s

void clang_analyzer_warnIfReached();

int *get();
void top() {
  int *p = get();
  int x = *p;
  // come code, but still within this function.

  // p is in a condition!
  if (p) {
    clang_analyzer_warnIfReached();
    return;
  }
  clang_analyzer_warnIfReached();
}
