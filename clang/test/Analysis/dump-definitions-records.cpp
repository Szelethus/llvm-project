// RUN: %clang_analyze_cc1 %s \
// RUN:   -analyzer-checker=debug.DumpCFG \
// RUN:   -analyzer-checker=debug.DumpGenSets \
// RUN:   -analyzer-checker=debug.DumpKillSets \
// RUN:   -analyzer-checker=debug.DumpReachingDefinitions \
// RUN:   2>&1 | FileCheck %s

int global_var;

namespace struct1 {

struct S {
  int a, b;
};

void structtest() {
  S s;
  s.a = 5;
}

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (s, [1, 1]) <write>
// CHECK-NEXT: 1 (s.a, [1, 5]) <write>
// CHECK-NEXT: 1 (s.b, [1, 1]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (s, [1, 1]) <write>
// CHECK-NEXT: 0 IN (s.a, [1, 5]) <write>
// CHECK-NEXT: 0 IN (s.b, [1, 1]) <write>
// CHECK-NEXT: 0 OUT (s, [1, 1]) <write>
// CHECK-NEXT: 0 OUT (s.a, [1, 5]) <write>
// CHECK-NEXT: 0 OUT (s.b, [1, 1]) <write>
// CHECK-NEXT: 1 OUT (s, [1, 1]) <write>
// CHECK-NEXT: 1 OUT (s.a, [1, 5]) <write>
// CHECK-NEXT: 1 OUT (s.b, [1, 1]) <write>

void struct_param(S s) {
  s.a = 5;
}

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (s.a, [1, 3]) <write>
// CHECK-NEXT: 2 (s, [2, 0]) <write>
// CHECK-NEXT: 2 (s.a, [2, 0]) <write>
// CHECK-NEXT: 2 (s.b, [2, 0]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (s.a, [2, 0]) <write>
// CHECK-NEXT: 2 (s.a, [1, 3]) <write>
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (s, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.a, [1, 3]) <write>
// CHECK-NEXT: 0 IN (s.b, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.a, [1, 3]) <write>
// CHECK-NEXT: 0 OUT (s.b, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.a, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.b, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.a, [1, 3]) <write>
// CHECK-NEXT: 1 OUT (s.b, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.a, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.b, [2, 0]) <write>

} // end of namespace struct1

namespace struct2 {

struct Inner {
  int a, b;
};

struct S {
  Inner x;
  Inner y;
};

void struct_param(S s) {
  s.x.b = 7;
  s.y.b = 5;
}

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (s.x.b, [1, 4]) <write>
// CHECK-NEXT: 1 (s.y.b, [1, 9]) <write>
// CHECK-NEXT: 2 (s, [2, 0]) <write>
// CHECK-NEXT: 2 (s.x, [2, 0]) <write>
// CHECK-NEXT: 2 (s.x.a, [2, 0]) <write>
// CHECK-NEXT: 2 (s.x.b, [2, 0]) <write>
// CHECK-NEXT: 2 (s.y, [2, 0]) <write>
// CHECK-NEXT: 2 (s.y.a, [2, 0]) <write>
// CHECK-NEXT: 2 (s.y.b, [2, 0]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (s.x.b, [2, 0]) <write>
// CHECK-NEXT: 1 (s.y.b, [2, 0]) <write>
// CHECK-NEXT: 2 (s.x.b, [1, 4]) <write>
// CHECK-NEXT: 2 (s.y.b, [1, 9]) <write>
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (s, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.x, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.x.a, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.x.b, [1, 4]) <write>
// CHECK-NEXT: 0 IN (s.y, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.y.a, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.y.b, [1, 9]) <write>
// CHECK-NEXT: 0 OUT (s, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.x, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.x.a, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.x.b, [1, 4]) <write>
// CHECK-NEXT: 0 OUT (s.y, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.y.a, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.y.b, [1, 9]) <write>
// CHECK-NEXT: 1 IN (s, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.x, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.x.a, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.x.b, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.y, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.y.a, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.y.b, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.x, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.x.a, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.x.b, [1, 4]) <write>
// CHECK-NEXT: 1 OUT (s.y, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.y.a, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.y.b, [1, 9]) <write>
// CHECK-NEXT: 2 OUT (s, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.x, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.x.a, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.x.b, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.y, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.y.a, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.y.b, [2, 0]) <write>

} // namespace struct2

namespace struct_ptr {

struct Inner {
  int a, b;
};

struct S {
  Inner *x;
  Inner &y;
  Inner z;
  S();
};

void struct_param(S s) {
  s.x->b = 7;
  s.y.b = 5;
  s.z.a = 5;
}

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (s.z.a, [1, 15]) <write>
// CHECK-NEXT: 2 (s, [2, 0]) <write>
// CHECK-NEXT: 2 (s.x, [2, 0]) <write>
// CHECK-NEXT: 2 (s.y, [2, 0]) <write>
// CHECK-NEXT: 2 (s.z, [2, 0]) <write>
// CHECK-NEXT: 2 (s.z.a, [2, 0]) <write>
// CHECK-NEXT: 2 (s.z.b, [2, 0]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (s.z.a, [2, 0]) <write>
// CHECK-NEXT: 2 (s.z.a, [1, 15]) <write>
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (s, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.x, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.y, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.z, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.z.a, [1, 15]) <write>
// CHECK-NEXT: 0 IN (s.z.b, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.x, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.y, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.z, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.z.a, [1, 15]) <write>
// CHECK-NEXT: 0 OUT (s.z.b, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.x, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.y, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.z, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.z.a, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.z.b, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.x, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.y, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.z, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.z.a, [1, 15]) <write>
// CHECK-NEXT: 1 OUT (s.z.b, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.x, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.y, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.z, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.z.a, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.z.b, [2, 0]) <write>

void struct_switten(S s) {
  s.x->b = 7;
  s.y = {};
  s.z.a = 5;
}
// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (global_var, [1, 14]) <invalidation>
// CHECK-NEXT: 1 (s.z.a, [1, 19]) <write>
// CHECK-NEXT: 2 (s, [2, 0]) <write>
// CHECK-NEXT: 2 (s.x, [2, 0]) <write>
// CHECK-NEXT: 2 (s.y, [2, 0]) <write>
// CHECK-NEXT: 2 (s.z, [2, 0]) <write>
// CHECK-NEXT: 2 (s.z.a, [2, 0]) <write>
// CHECK-NEXT: 2 (s.z.b, [2, 0]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (s.z.a, [2, 0]) <write>
// CHECK-NEXT: 2 (s.z.a, [1, 19]) <write>
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (global_var, [1, 14]) <invalidation>
// CHECK-NEXT: 0 IN (s, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.x, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.y, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.z, [2, 0]) <write>
// CHECK-NEXT: 0 IN (s.z.a, [1, 19]) <write>
// CHECK-NEXT: 0 IN (s.z.b, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (global_var, [1, 14]) <invalidation>
// CHECK-NEXT: 0 OUT (s, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.x, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.y, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.z, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (s.z.a, [1, 19]) <write>
// CHECK-NEXT: 0 OUT (s.z.b, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.x, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.y, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.z, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.z.a, [2, 0]) <write>
// CHECK-NEXT: 1 IN (s.z.b, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (global_var, [1, 14]) <invalidation>
// CHECK-NEXT: 1 OUT (s, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.x, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.y, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.z, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (s.z.a, [1, 19]) <write>
// CHECK-NEXT: 1 OUT (s.z.b, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.x, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.y, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.z, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.z.a, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (s.z.b, [2, 0]) <write>

} // namespace struct_ptr
