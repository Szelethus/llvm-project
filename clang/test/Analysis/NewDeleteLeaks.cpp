// RUN: %clang_analyze_cc1 -analyzer-checker=core,cplusplus,unix -verify -analyzer-output=text %s

#include "Inputs/system-header-simulator-for-malloc.h"

//===----------------------------------------------------------------------===//
// Report for which we expect NoOwnershipChangeVisitor to add a new note.
//===----------------------------------------------------------------------===//

bool coin();

namespace memory_allocated_in_fn_call {

void sink(int *P) {
} // expected-note {{Returning without changing the ownership status of allocated memory}}

void foo() {
  sink(new int(5)); // expected-note {{Memory is allocated}}
                    // expected-note@-1 {{Calling 'sink'}}
                    // expected-note@-2 {{Returning from 'sink'}}
} // expected-warning {{Potential memory leak [cplusplus.NewDeleteLeaks]}}
// expected-note@-1 {{Potential memory leak}}

} // namespace memory_allocated_in_fn_call

namespace memory_passed_to_fn_call {

void sink(int *P) {
  if (coin()) // expected-note {{Assuming the condition is false}}
              // expected-note@-1 {{Taking false branch}}
    delete P;
} // expected-note {{Returning without changing the ownership status of allocated memory}}

void foo() {
  int *ptr = new int(5); // expected-note {{Memory is allocated}}
  sink(ptr);             // expected-note {{Calling 'sink'}}
                         // expected-note@-1 {{Returning from 'sink'}}
} // expected-warning {{Potential leak of memory pointed to by 'ptr' [cplusplus.NewDeleteLeaks]}}
// expected-note@-1 {{Potential leak}}

} // namespace memory_passed_to_fn_call

//===----------------------------------------------------------------------===//
// Report for which we *do not* expect NoOwnershipChangeVisitor add a new note,
// nor do we want it to.
//===----------------------------------------------------------------------===//

namespace memory_not_passed_to_fn_call {

// TODO: We don't want a note here. We need to check whether the allocated
// memory was actually passed into the function.
void sink(int *P) {
  if (coin()) // expected-note {{Assuming the condition is false}}
              // expected-note@-1 {{Taking false branch}}
    delete P;
} // expected-note {{Returning without changing the ownership status of allocated memory}}

void foo() {
  int *ptr = new int(5); // expected-note {{Memory is allocated}}
  int *q = nullptr;
  sink(q); // expected-note {{Calling 'sink'}}
            // expected-note@-1 {{Returning from 'sink'}}
  (void)ptr;
} // expected-warning {{Potential leak of memory pointed to by 'ptr' [cplusplus.NewDeleteLeaks]}}
// expected-note@-1 {{Potential leak}}

} // namespace memory_not_passed_to_fn_call

// TODO: We don't want a note here. sink() doesn't seem like a function that
// even attempts to take care of any memory ownership problems.
namespace memory_passed_into_fn_that_doesnt_intend_to_free {

void sink(int *P) {
} // expected-note {{Returning without changing the ownership status of allocated memory}}

void foo() {
  int *ptr = new int(5); // expected-note {{Memory is allocated}}
  sink(ptr);             // expected-note {{Calling 'sink'}}
                         // expected-note@-1 {{Returning from 'sink'}}
} // expected-warning {{Potential leak of memory pointed to by 'ptr' [cplusplus.NewDeleteLeaks]}}
// expected-note@-1 {{Potential leak}}

} // namespace memory_passed_into_fn_that_doesnt_intend_to_free

namespace refkind_from_unoallocated_to_allocated {

// RefKind of the symbol changed from nothing to Allocated. We don't want to
// emit notes when the RefKind changes in the stack frame.
static char *malloc_wrapper_ret() {
  return (char *)malloc(12); // expected-note {{Memory is allocated}}
}
void use_ret() {
  char *v;
  v = malloc_wrapper_ret(); // expected-note {{Calling 'malloc_wrapper_ret'}}
                            // expected-note@-1 {{Returned allocated memory}}
} // expected-warning {{Potential leak of memory pointed to by 'v' [unix.Malloc]}}
// expected-note@-1 {{Potential leak of memory pointed to by 'v'}}

} // namespace refkind_from_unoallocated_to_allocated
