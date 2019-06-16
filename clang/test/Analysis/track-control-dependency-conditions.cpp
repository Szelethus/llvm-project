// RUN: %clang_analyze_cc1 %s -verify \
// RUN:   -analyzer-config track-conditions=true \
// RUN:   -analyzer-output=text \
// RUN:   -analyzer-checker=core

namespace example_1 {
int flag;
bool coin();

void foo() {
  flag = coin(); // expected-note {{Value assigned to 'flag'}}
}

void test() {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}
  flag = 1;

  foo(); // TODO: Add nodes here about flag's value being invalidated.
  if (flag) // expected-note   {{Assuming 'flag' is 0}}
            // expected-note@-1{{Taking false branch}}
    x = new int;

  foo(); // expected-note   {{Calling 'foo'}}
         // expected-note@-1{{Returning from 'foo'}}

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
  flag = coin(); // expected-note {{Value assigned to 'flag'}}
}

void test() {
  int *x = 0;
  flag = 1;

  foo();
  if (flag) // expected-note   {{Assuming 'flag' is 0}}
            // expected-note@-1{{Taking false branch}}
    x = new int;

  x = 0; // expected-note{{Null pointer value stored to 'x'}}

  foo(); // expected-note   {{Calling 'foo'}}
         // expected-note@-1{{Returning from 'foo'}}

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
  flag = coin(); // expected-note {{Value assigned to 'flag'}}
                 // expected-note@-1 {{Value assigned to 'bar'}}
}

int bar;

void test() {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}
  flag = 1;

  foo(); // expected-note   {{Calling 'foo'}}
         // expected-note@-1{{Returning from 'foo'}}

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
  return coin(); // expected-note {{Returning value}}
}

int bar;

void test() {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}

  if (int flag = foo()) // expected-note {{Calling 'foo'}}
            // expected-note@-1{{Returning from 'foo'}}
            // expected-note@-2{{'flag' initialized here}}
            // expected-note@-3{{Assuming 'flag' is not equal to 0}}
            // expected-note@-4{{Taking true branch}}
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}
} // end of namespace variable_declaration_in_condition

namespace conversion_to_bool {
bool coin();

struct ConvertsToBool {
  operator bool() const { return coin(); } // expected-note {{Returning value}}
};

void test() {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}

  if (ConvertsToBool()) // expected-note {{Calling 'ConvertsToBool::operator bool'}}
            // expected-note@-1{{Returning from 'ConvertsToBool::operator bool'}}
            // expected-note@-2{{Assuming the condition is true}}
            // expected-note@-3{{Taking true branch}}
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}

} // end of namespace variable_declaration_in_condition
