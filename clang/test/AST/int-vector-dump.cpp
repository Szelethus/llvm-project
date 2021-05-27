// RUN: %clang_cc1 -int-vector-dump %s | FileCheck %s

void f() {

  for(;;)
    ;
}

void g() {
  f();
  if(int())
    ;
}

// CHECK: f
// CHECK: g
