// UN: %clang_analyze_cc1 -verify %s -analyzer-output=text\
// UN:   -analyzer-checker=core \
// UN:   -analyzer-checker=alpha.core.NullPtrInterference

struct A {
  void a() {
    if (this)
      ;
  }
};
