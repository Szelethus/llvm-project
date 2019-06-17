// RUN: %clang_analyze_cc1 %s -verify -DTRACKING_CONDITIONS \
// RUN:   -analyzer-config track-conditions=true \
// RUN:   -analyzer-output=text \
// RUN:   -analyzer-checker=core
//
// RUN: %clang_analyze_cc1 %s -verify \
// RUN:   -analyzer-output=text \
// RUN:   -analyzer-checker=core

namespace example_1 {
int flag;
bool coin();

void foo() {
  flag = coin();
#ifdef TRACKING_CONDITIONS
  // expected-note@-2{{Value assigned to 'flag'}}
#endif // TRACKING_CONDITIONS
}

void test() {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}
  flag = 1;

  foo(); // TODO: Add nodes here about flag's value being invalidated.
  if (flag) // expected-note   {{Assuming 'flag' is 0}}
            // expected-note@-1{{Taking false branch}}
    x = new int;

  foo();
#ifdef TRACKING_CONDITIONS
  // expected-note@-2{{Calling 'foo'}}
  // expected-note@-3{{Returning from 'foo'}}
#endif // TRACKING_CONDITIONS

  if (flag) // expected-note   {{Assuming 'flag' is not equal to 0}}
            // expected-note@-1{{Taking true branch}}
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}
} // end of namespace example_1

namespace example_2 {
int flag;
bool coin();

void foo() {
  flag = coin();
#ifdef TRACKING_CONDITIONS
  // expected-note@-2{{Value assigned to 'flag'}}
#endif // TRACKING_CONDITIONS
}

void test() {
  int *x = 0;
  flag = 1;

  foo();
  if (flag) // expected-note   {{Assuming 'flag' is 0}}
            // expected-note@-1{{Taking false branch}}
    x = new int;

  x = 0; // expected-note{{Null pointer value stored to 'x'}}

  foo();
#ifdef TRACKING_CONDITIONS
  // expected-note@-2{{Calling 'foo'}}
  // expected-note@-3{{Returning from 'foo'}}
#endif // TRACKING_CONDITIONS

  if (flag) // expected-note   {{Assuming 'flag' is not equal to 0}}
            // expected-note@-1{{Taking true branch}}
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}
} // end of namespace example_2

namespace example_3 {
int flag;
bool coin();

void foo() {
  // coin() could write bar, do it's invalidated.
  flag = coin();
#ifdef TRACKING_CONDITIONS
  // expected-note@-2{{Value assigned to 'flag'}}
  // expected-note@-3{{Value assigned to 'bar'}}
#endif // TRACKING_CONDITIONS
}

int bar;

void test() {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}
  flag = 1;

  foo();
#ifdef TRACKING_CONDITIONS
  // expected-note@-2{{Calling 'foo'}}
  // expected-note@-3{{Returning from 'foo'}}
#endif // TRACKING_CONDITIONS

  if (bar) // expected-note   {{Assuming 'bar' is not equal to 0}}
           // expected-note@-1{{Taking true branch}}
    if (flag) // expected-note   {{Assuming 'flag' is not equal to 0}}
              // expected-note@-1{{Taking true branch}}
      *x = 5; // expected-warning{{Dereference of null pointer}}
              // expected-note@-1{{Dereference of null pointer}}
}
} // end of namespace example_3

namespace variable_declaration_in_condition {
bool coin();

bool foo() {
  return coin();
#ifdef TRACKING_CONDITIONS
  // expected-note@-2{{Returning value}}
#endif // TRACKING_CONDITIONS
}

int bar;

void test() {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}

  if (int flag = foo())
#ifdef TRACKING_CONDITIONS
    // expected-note@-2{{Calling 'foo'}}
    // expected-note@-3{{Returning from 'foo'}}
    // expected-note@-4{{'flag' initialized here}}
#endif // TRACKING_CONDITIONS
    // expected-note@-6{{Assuming 'flag' is not equal to 0}}
    // expected-note@-7{{Taking true branch}}
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}
} // end of namespace variable_declaration_in_condition

namespace conversion_to_bool {
bool coin();

struct ConvertsToBool {
  operator bool() const { return coin(); }
#ifdef TRACKING_CONDITIONS
  // expected-note@-2{{Returning value}}
#endif // TRACKING_CONDITIONS
};

void test() {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}

  if (ConvertsToBool())
#ifdef TRACKING_CONDITIONS
    // expected-note@-2 {{Calling 'ConvertsToBool::operator bool'}}
    // expected-note@-3{{Returning from 'ConvertsToBool::operator bool'}}
#endif // TRACKING_CONDITIONS
    // expected-note@-5{{Assuming the condition is true}}
    // expected-note@-6{{Taking true branch}}
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}

} // end of namespace variable_declaration_in_condition
