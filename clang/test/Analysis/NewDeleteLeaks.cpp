// RUN: %clang_analyze_cc1 -analyzer-checker=core,cplusplus,unix -verify -analyzer-output=text %s
int *ptr;

void sink(int *P) {
} // expected-note {{Returning without changing the ownership status of allocated memory}}

void f() {
  sink(new int(5)); // expected-note {{Memory is allocated}}
                    // expected-note@-1 {{Calling 'sink'}}
                    // expected-note@-2 {{Returning from 'sink'}}
} // expected-warning {{Potential memory leak [cplusplus.NewDeleteLeaks]}}
  // expected-note@-1 {{Potential memory leak}}

typedef __typeof(sizeof(int)) size_t;
void *malloc(size_t);

// RefKind of the symbol changed from nothing to Allocated. We don't want to
// emit notes when the RefKind changes in the stack frame.
static char *malloc_wrapper_ret() {
    return (char*)malloc(12); // expected-note {{Memory is allocated}}
}
void use_ret() {
    char *v;
    v = malloc_wrapper_ret(); // expected-note {{Calling 'malloc_wrapper_ret'}}
                              // expected-note@-1 {{Returned allocated memory}}
} // expected-warning {{Potential leak of memory pointed to by 'v' [unix.Malloc]}}
  // expected-note@-1 {{Potential leak of memory pointed to by 'v'}}
