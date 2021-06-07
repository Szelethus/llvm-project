// RUN: %clang_cc1 -int-vector-dump %s | FileCheck %s

void f() {

  for(;;)
    ;
}

void g() {
  f();
  int a, s, d, f, g, h, j, k, l, y, x, c, v, b, n, m, q, w, e, r, t, z, u, ki, o, p;
  using hello = int;
  using bello = int;
  using sello = int;
  using fello = int;
  using rello = int;
  using jello = int;
  if(int())
    ;
  int *i = new int;
  delete i;
}

// CHECK: Function name,ForStmt count,IfStmt count,CXXNewExpr count,CXXDeleteCount count,
// CHECK: f,1,0,0,0,
// CHECK: g,0,1,1,0,
