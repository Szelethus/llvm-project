// RUN: %clang_analyze_cc1 -std=c++11 -fblocks -verify %s \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=debug.AnalysisOrder \
// RUN:   -analyzer-config debug.AnalysisOrder:PreStmtCXXDeleteExpr=true \
// RUN:   -analyzer-config debug.AnalysisOrder:PostStmtCXXDeleteExpr=true \
// RUN:   -analyzer-config debug.AnalysisOrder:PreCall=true \
// RUN:   -analyzer-config debug.AnalysisOrder:PostCall=true \
// RUN:   2>&1 | FileCheck %s

// expected-no-diagnostics

namespace fundamental_dealloc {
void f() {
  int *p = new int;
  delete p;
}
} // namespace fundamental_dealloc

namespace record_dealloc {
struct S {
  int x, y;
};

void f() {
  S *s = new S;
  delete s;
}
} // namespace record_dealloc

namespace nontrivial_destructor {
struct NontrivialDtor {
  int x, y;

  ~NontrivialDtor() {
    // Casually mine bitcoin.
  }
};

void f() {
  NontrivialDtor *s = new NontrivialDtor;
  delete s;
}
} // namespace nontrivial_destructor

// Mind that the results here are in reverse order compared how functions are
// defined in this file.

// CHECK:      PreCall (operator new)
// CHECK-NEXT: PostCall (operator new)
// CHECK-NEXT: PreCall (nontrivial_destructor::NontrivialDtor::NontrivialDtor)
// CHECK-NEXT: PostCall (nontrivial_destructor::NontrivialDtor::NontrivialDtor)
// CHECK-NEXT: PreCall (nontrivial_destructor::NontrivialDtor::~NontrivialDtor)
// CHECK-NEXT: PostCall (nontrivial_destructor::NontrivialDtor::~NontrivialDtor)
// CHECK-NEXT: PreStmt<CXXDeleteExpr>
// CHECK-NEXT: PreCall (operator delete)

// CHECK:      PreCall (operator new)
// CHECK-NEXT: PostCall (operator new)
// CHECK-NEXT: PreCall (record_dealloc::S::S)
// CHECK-NEXT: PostCall (record_dealloc::S::S)
// CHECK-NEXT: PreStmt<CXXDeleteExpr>
// CHECK-NEXT: PreCall (operator delete)

// CHECK:      PreCall (operator new)
// CHECK-NEXT: PostCall (operator new)
// CHECK-NEXT: PreStmt<CXXDeleteExpr>
// CHECK-NEXT: PreCall (operator delete)
