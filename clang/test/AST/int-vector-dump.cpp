// RUN: %clang_cc1 -int-vector-dump %s | FileCheck %s

void f() {

  for(;;)
    ;
}

void g() {
  f();
  if(int())
    ;
  int *i = new int;
}

// CHECK: Function name,ForStmt count,IfStmt count,CXXNewExpr count,
// CHECK: f,1,0,0,
// CHECK: g,0,1,1,
