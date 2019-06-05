// RUN: %clang_analyze_cc1 -std=c++11 -verify %s \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=optin.cplusplus.UninitializedObject \
// RUN:   -analyzer-config \
// RUN:     optin.cplusplus.UninitializedObject:Pedantic=true \
// RUN:   -analyzer-output=text

namespace must_have_happened {
struct S {
  int x; // expected-note{{uninitialized field 'this->x'}}
  S(int r) {
    if (r) // expected-note{{Assuming 'r' is 0}}
           // expected-note@-1{{Taking false branch}}
      x = 5;
  } // expected-warning{{1 uninitialized field}}
  // expected-note@-1{{1 uninitialized field}}
};

void f(int r) {
  S s(r); // expected-note{{Calling constructor for 'S'}}
}
} // end of namespace must_have_happened

namespace inlined_category {
void init(int cond, int &val) {
  if (cond)
    val = 0;
}

struct S {
  int x; // expected-note{{uninitialized field 'this->x'}}
  S(int r) {
    init(r, x);
  } // expected-warning{{1 uninitialized field}}
  // expected-note@-1{{1 uninitialized field}}
};

void f(int r) {
  S s(r); // expected-note{{Calling constructor for 'S'}}
}
} // end of namespace inlined_category

namespace not_inlined_category {
void init(int cond, int &val) {
  if (cond)
    val = 0;
}

struct S {
  int x; // expected-note{{uninitialized field 'this->x'}}
  S(int r) {
    if (r) // expected-note{{Assuming 'r' is 0}}
           // expected-note@-1{{Taking false branch}}
      init(r, x);
  } // expected-warning{{1 uninitialized field}}
  // expected-note@-1{{1 uninitialized field}}
};

void f(int r) {
  S s(r); // expected-note{{Calling constructor for 'S'}}
}
} // end of namespace not_inlined_category
