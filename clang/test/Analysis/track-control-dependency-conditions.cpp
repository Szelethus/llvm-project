// RUN: %clang_analyze_cc1 %s -verify \
// RUN:   -analyzer-output=text \
// RUN:   -analyzer-checker=core

int flag;
bool coin();

void foo() {
  flag = coin(); // expected-note 2{{Value assigned to 'flag'}}
}

void example_1() {
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

void example_2() {
  int *x = 0;
  flag = 1;

  foo(); // TODO: Add nodes here about flag's value being invalidated.
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
