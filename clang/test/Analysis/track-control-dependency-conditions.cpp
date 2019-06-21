// RUN: %clang_analyze_cc1 %s -verify -DTRACKING_CONDITIONS \
// RUN:   -analyzer-config track-conditions=true \
// RUN:   -analyzer-output=text \
// RUN:   -analyzer-checker=core

// RUN: not %clang_analyze_cc1 -verify %s \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-config track-conditions-debug=true \
// RUN:   2>&1 | FileCheck %s -check-prefix=CHECK-INVALID-DEBUG

// CHECK-INVALID-DEBUG: (frontend): invalid input for analyzer-config option
// CHECK-INVALID-DEBUG-SAME:        'track-conditions-debug', that expects
// CHECK-INVALID-DEBUG-SAME:        'track-conditions' to also be enabled
//
// RUN: %clang_analyze_cc1 %s -verify \
// RUN:   -DTRACKING_CONDITIONS -DTRACKING_CONDITIONS_DEBUG \
// RUN:   -analyzer-config track-conditions=true \
// RUN:   -analyzer-config track-conditions-debug=true \
// RUN:   -analyzer-output=text \
// RUN:   -analyzer-checker=core

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
#ifdef TRACKING_CONDITIONS_DEBUG
            // expected-note@-3{{Tracking condition 'flag'}}
#endif // TRACKING_CONDITIONS_DEBUG
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
#ifdef TRACKING_CONDITIONS_DEBUG
            // expected-note@-3{{Tracking condition 'flag'}}
#endif // TRACKING_CONDITIONS_DEBUG
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}
} // end of namespace example_2

namespace global_variable_invalidation {
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
#ifdef TRACKING_CONDITIONS_DEBUG
           // expected-note@-3{{Tracking condition 'bar'}}
#endif // TRACKING_CONDITIONS_DEBUG
    if (flag) // expected-note   {{Assuming 'flag' is not equal to 0}}
              // expected-note@-1{{Taking true branch}}
#ifdef TRACKING_CONDITIONS_DEBUG
              // expected-note@-3{{Tracking condition 'flag'}}
#endif // TRACKING_CONDITIONS_DEBUG
      *x = 5; // expected-warning{{Dereference of null pointer}}
              // expected-note@-1{{Dereference of null pointer}}
}
} // end of namespace global_variable_invalidation

namespace variable_declaration_in_condition {
bool coin();

bool foo() {
  return coin();
}

int bar;

void test() {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}

  if (int flag = foo())
#ifdef TRACKING_CONDITIONS_DEBUG
    // expected-note@-2{{Tracking condition 'flag'}}
#endif // TRACKING_CONDITIONS_DEBUG
    // expected-note@-4{{Assuming 'flag' is not equal to 0}}
    // expected-note@-5{{Taking true branch}}

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
#ifdef TRACKING_CONDITIONS_DEBUG
    // expected-note@-5{{Tracking condition 'ConvertsToBool()'}}
#endif // TRACKING_CONDITIONS_DEBUG
#endif // TRACKING_CONDITIONS
    // expected-note@-8{{Assuming the condition is true}}
    // expected-note@-9{{Taking true branch}}
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}

} // end of namespace variable_declaration_in_condition

namespace tracked_condition_is_only_initialized {
int getInt();

void f() {
  int flag = getInt();
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}
  if (flag) // expected-note{{Assuming 'flag' is not equal to 0}}
            // expected-note@-1{{Taking true branch}}
#ifdef TRACKING_CONDITIONS_DEBUG
            // expected-note@-3{{Tracking condition 'flag'}}
#endif // TRACKING_CONDITIONS_DEBUG
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}
} // end of namespace tracked_condition_is_only_initialized

namespace tracked_condition_written_in_same_stackframe {
int flag;
int getInt();

void f(int y) {
  y = 1;
  flag = y;
#ifdef TRACKING_CONDITIONS
  // expected-note@-2{{The value 1 is assigned to 'flag'}}
#endif // TRACKING_CONDITIONS
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}
  if (flag) // expected-note{{'flag' is 1}}
            // expected-note@-1{{Taking true branch}}
#ifdef TRACKING_CONDITIONS_DEBUG
            // expected-note@-3{{Tracking condition 'flag'}}
#endif // TRACKING_CONDITIONS_DEBUG
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}
} // end of namespace tracked_condition_written_in_same_stackframe

namespace tracked_condition_written_in_nested_stackframe {
int flag;
int getInt();

void foo() {
  int y;
  y = 1;
  flag = y;
#ifdef TRACKING_CONDITIONS
  // expected-note@-2{{The value 1 is assigned to 'flag'}}
#endif // TRACKING_CONDITIONS

}

void f(int y) {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}
  foo();
#ifdef TRACKING_CONDITIONS
  // expected-note@-2{{Calling 'foo'}}
  // expected-note@-3{{Returning from 'foo'}}
#endif // TRACKING_CONDITIONS
  if (flag) // expected-note{{'flag' is 1}}
            // expected-note@-1{{Taking true branch}}
#ifdef TRACKING_CONDITIONS_DEBUG
            // expected-note@-3{{Tracking condition 'flag'}}
#endif // TRACKING_CONDITIONS_DEBUG
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}
} // end of namespace tracked_condition_written_in_nested_stackframe

namespace collapse_point_not_in_condition {

[[noreturn]] void halt();

void assert(int b) {
  if (!b)
#ifdef TRACKING_CONDITIONS
    // expected-note@-2{{Assuming 'b' is not equal to 0}}
    // expected-note@-3{{Taking false branch}}
#endif // TRACKING_CONDITIONS
    halt();
}

void f(int flag) {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}
  assert(flag);
#ifdef TRACKING_CONDITIONS
  // expected-note@-2{{Calling 'assert'}}
  // expected-note@-3{{Returning from 'assert'}}
#endif // TRACKING_CONDITIONS
  if (flag) // expected-note{{'flag' is not equal to 0}}
            // expected-note@-1{{Taking true branch}}
#ifdef TRACKING_CONDITIONS_DEBUG
            // expected-note@-3{{Tracking condition 'flag'}}
#endif // TRACKING_CONDITIONS_DEBUG
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}

} // end of namespace collapse_point_not_in_condition

namespace unimportant_write_before_collapse_point {

[[noreturn]] void halt();

void assert(int b) {
  if (!b)
#ifdef TRACKING_CONDITIONS
    // expected-note@-2{{Assuming 'b' is not equal to 0}}
    // expected-note@-3{{Taking false branch}}
#endif // TRACKING_CONDITIONS
    halt();
}
int getInt();

void f(int flag) {
  int *x = 0; // expected-note{{'x' initialized to a null pointer value}}
  flag = getInt();
  assert(flag);
#ifdef TRACKING_CONDITIONS
  // expected-note@-3{{Value assigned to 'flag'}}
  // expected-note@-3{{Calling 'assert'}}
  // expected-note@-4{{Returning from 'assert'}}
#endif // TRACKING_CONDITIONS
  if (flag) // expected-note{{'flag' is not equal to 0}}
            // expected-note@-1{{Taking true branch}}
#ifdef TRACKING_CONDITIONS_DEBUG
            // expected-note@-3{{Tracking condition 'flag'}}
#endif // TRACKING_CONDITIONS_DEBUG
    *x = 5; // expected-warning{{Dereference of null pointer}}
            // expected-note@-1{{Dereference of null pointer}}
}

} // end of namespace unimportant_write_before_collapse_point
