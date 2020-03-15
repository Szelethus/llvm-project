// RUN: %clang_analyze_cc1 %s \
// RUN:   -analyzer-checker=debug.DumpCFG \
// RUN:   -analyzer-checker=debug.DumpGenSets \
// RUN:   -analyzer-checker=debug.DumpKillSets \
// RUN:   -analyzer-checker=debug.DumpReachingDefinitions \
// RUN:   2>&1 | FileCheck %s

void lhs_in_parantheses(int a, int b) {
  (a, b) = 5;
}
// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 2 (a, [2, 0]) <write>
// CHECK-NEXT: 2 (b, [2, 0]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (a, [2, 0]) <write>
// CHECK-NEXT: 0 IN (b, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (a, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (b, [2, 0]) <write>
// CHECK-NEXT: 1 IN (a, [2, 0]) <write>
// CHECK-NEXT: 1 IN (b, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (a, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (b, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (a, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (b, [2, 0]) <write>

void lhs_buried_in_parantheses(int a, int b) {
  ((((((((a, b)))))))) = 5;
}
// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 2 (a, [2, 0]) <write>
// CHECK-NEXT: 2 (b, [2, 0]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (a, [2, 0]) <write>
// CHECK-NEXT: 0 IN (b, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (a, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (b, [2, 0]) <write>
// CHECK-NEXT: 1 IN (a, [2, 0]) <write>
// CHECK-NEXT: 1 IN (b, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (a, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (b, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (a, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (b, [2, 0]) <write>

void lhs_buried_in_parantheses2(int a, int b) {
  ((((((((a))))), b))) = 5;
}
// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 2 (a, [2, 0]) <write>
// CHECK-NEXT: 2 (b, [2, 0]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (a, [2, 0]) <write>
// CHECK-NEXT: 0 IN (b, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (a, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (b, [2, 0]) <write>
// CHECK-NEXT: 1 IN (a, [2, 0]) <write>
// CHECK-NEXT: 1 IN (b, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (a, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (b, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (a, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (b, [2, 0]) <write>

void lhs_is_conditional_operator(int a, int b) {
  (a ? a : b) = 5;
}
// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 5 (a, [5, 0]) <write>
// CHECK-NEXT: 5 (b, [5, 0]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (a, [5, 0]) <write>
// CHECK-NEXT: 0 IN (b, [5, 0]) <write>
// CHECK-NEXT: 0 OUT (a, [5, 0]) <write>
// CHECK-NEXT: 0 OUT (b, [5, 0]) <write>
// CHECK-NEXT: 1 IN (a, [5, 0]) <write>
// CHECK-NEXT: 1 IN (b, [5, 0]) <write>
// CHECK-NEXT: 1 OUT (a, [5, 0]) <write>
// CHECK-NEXT: 1 OUT (b, [5, 0]) <write>
// CHECK-NEXT: 2 IN (a, [5, 0]) <write>
// CHECK-NEXT: 2 IN (b, [5, 0]) <write>
// CHECK-NEXT: 2 OUT (a, [5, 0]) <write>
// CHECK-NEXT: 2 OUT (b, [5, 0]) <write>
// CHECK-NEXT: 3 IN (a, [5, 0]) <write>
// CHECK-NEXT: 3 IN (b, [5, 0]) <write>
// CHECK-NEXT: 3 OUT (a, [5, 0]) <write>
// CHECK-NEXT: 3 OUT (b, [5, 0]) <write>
// CHECK-NEXT: 4 IN (a, [5, 0]) <write>
// CHECK-NEXT: 4 IN (b, [5, 0]) <write>
// CHECK-NEXT: 4 OUT (a, [5, 0]) <write>
// CHECK-NEXT: 4 OUT (b, [5, 0]) <write>
// CHECK-NEXT: 5 OUT (a, [5, 0]) <write>
// CHECK-NEXT: 5 OUT (b, [5, 0]) <write>
