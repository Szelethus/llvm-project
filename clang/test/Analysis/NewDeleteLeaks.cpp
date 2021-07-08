// RUN: %clang_analyze_cc1 -analyzer-checker=core,cplusplus,unix -verify -analyzer-output=text %s

#include "Inputs/system-header-simulator-for-malloc.h"

//===----------------------------------------------------------------------===//
// Report for which NoOwnershipChangeVisitor added a new note.
//===----------------------------------------------------------------------===//

void sink(int *P) {
} // expected-note {{Returning without changing the ownership status of allocated memory}}

void memoryAllocatedInFnCall() {
  sink(new int(5)); // expected-note {{Memory is allocated}}
                    // expected-note@-1 {{Calling 'sink'}}
                    // expected-note@-2 {{Returning from 'sink'}}
} // expected-warning {{Potential memory leak [cplusplus.NewDeleteLeaks]}}
// expected-note@-1 {{Potential memory leak}}

//===----------------------------------------------------------------------===//
// Report for which NoOwnershipChangeVisitor *did not* add a new note, nor
// do we want it to.
//===----------------------------------------------------------------------===//

// TODO: We don't want a note here.
void sink2(int *P) {
} // expected-note {{Returning without changing the ownership status of allocated memory}}

void allocatedMemoryWasntPassed() {
  int *ptr = new int(5); // expected-note {{Memory is allocated}}
  int *q = nullptr;
  sink2(q); // expected-note {{Calling 'sink2'}}
            // expected-note@-1 {{Returning from 'sink2'}}
  (void)ptr;
} // expected-warning {{Potential leak of memory pointed to by 'ptr' [cplusplus.NewDeleteLeaks]}}
// expected-note@-1 {{Potential leak}}

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
