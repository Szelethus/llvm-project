// RUN: %clang_analyze_cc1 %s -verify \
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
  if (flag) // expected-note   {{Taking false branch}}
            // expected-note@-1{{Assuming 'flag' is 0}}
    x = new int;

  foo(); // expected-note   {{Calling 'foo'}}
         // expected-note@-1{{Returning from 'foo'}}

  if (flag) // expected-note   {{Taking true branch}}
            // expected-note@-1{{Assuming 'flag' is not equal to 0}}
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
  if (flag) // expected-note   {{Taking false branch}}
            // expected-note@-1{{Assuming 'flag' is 0}}
    x = new int;

  x = 0; // expected-note{{Null pointer value stored to 'x'}}

  foo(); // expected-note   {{Calling 'foo'}}
         // expected-note@-1{{Returning from 'foo'}}

  if (flag) // expected-note   {{Taking true branch}}
            // expected-note@-1{{Assuming 'flag' is not equal to 0}}
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}
} // end of namespace example_2

namespace example_3 {
int flag;
bool coin();

void foo() {
  // FIXME: It makes no sense at all for bar to have been assigned here.
  flag = coin(); // expected-note {{Value assigned to 'flag'}}
                 // expected-note@-1 {{Value assigned to 'bar'}}
}

int bar;

void test() {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}
  flag = 1;

  foo(); // expected-note   {{Calling 'foo'}}
         // expected-note@-1{{Returning from 'foo'}}

  if (bar) // expected-note   {{Taking true branch}}
           // expected-note@-1{{Assuming 'bar' is not equal to 0}}
    if (flag) // expected-note   {{Taking true branch}}
              // expected-note@-1{{Assuming 'flag' is not equal to 0}}
      *x = 5; // expected-warning{{Dereference of null pointer}}
              // expected-note@-1{{Dereference of null pointer}}
}
} // end of namespace example_3

namespace variable_declaration_in_condition {
bool coin();

bool foo() {
  // FIXME: It makes no sense at all for bar to have been assigned here.
  return coin(); // expected-note {{Value assigned to 'flag'}}
                 // expected-note@-1 {{Value assigned to 'bar'}}
}

int bar;

void test() {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}

  if (int flag = foo()) // expected-note   {{Taking true branch}}
            // expected-note@-1{{Assuming 'flag' is not equal to 0}}
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}
} // end of namespace variable_declaration_in_condition
