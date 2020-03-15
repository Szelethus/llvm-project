// RUN: %clang_analyze_cc1 %s \
// RUN:   -analyzer-checker=debug.DumpCFG \
// RUN:   -analyzer-checker=debug.DumpGenSets \
// RUN:   -analyzer-checker=debug.DumpKillSets \
// RUN:   -analyzer-checker=debug.DumpReachingDefinitions \
// RUN:   2>&1 | FileCheck %s

namespace simple_invalidation {

bool coin();

void invalidate(int &i, double d);

void invalidateOnFnCall() {
  int i;
  double d = 0.0;

  if (coin())
    i = 5;

  if (coin())
    d = 2.3;

  invalidate(i, d);
}
//                    -> [B4] ->    -> [B2] ->
//                   /          \  /          \
// [B6 (ENTRY)] -> [B5] ------> [B3] ------> [B1] -> [B0 (EXIT)]

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (global_var, [1, 5]) <invalidation>
// CHECK-NEXT: 1 (i, [1, 5]) <invalidation>
// CHECK-NEXT: 2 (d, [2, 2]) <write>
// CHECK-NEXT: 3 (global_var, [3, 2]) <invalidation>
// CHECK-NEXT: 4 (i, [4, 2]) <write>
// CHECK-NEXT: 5 (global_var, [5, 5]) <invalidation>
// CHECK-NEXT: 5 (i, [5, 0]) <write>
// CHECK-NEXT: 5 (d, [5, 2]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (global_var, [3, 2]) <invalidation>
// CHECK-NEXT: 1 (global_var, [5, 5]) <invalidation>
// CHECK-NEXT: 1 (i, [4, 2]) <write>
// CHECK-NEXT: 1 (i, [5, 0]) <write>
// CHECK-NEXT: 2 (d, [5, 2]) <write>
// CHECK-NEXT: 3 (global_var, [1, 5]) <invalidation>
// CHECK-NEXT: 3 (global_var, [5, 5]) <invalidation>
// CHECK-NEXT: 4 (i, [1, 5]) <invalidation>
// CHECK-NEXT: 4 (i, [5, 0]) <write>
// CHECK-NEXT: 5 (global_var, [1, 5]) <invalidation>
// CHECK-NEXT: 5 (global_var, [3, 2]) <invalidation>
// CHECK-NEXT: 5 (i, [1, 5]) <invalidation>
// CHECK-NEXT: 5 (i, [4, 2]) <write>
// CHECK-NEXT: 5 (d, [2, 2]) <write>
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (global_var, [1, 5]) <invalidation>
// CHECK-NEXT: 0 IN (i, [1, 5]) <invalidation>
// CHECK-NEXT: 0 IN (d, [2, 2]) <write>
// CHECK-NEXT: 0 IN (d, [5, 2]) <write>
// CHECK-NEXT: 0 OUT (global_var, [1, 5]) <invalidation>
// CHECK-NEXT: 0 OUT (i, [1, 5]) <invalidation>
// CHECK-NEXT: 0 OUT (d, [2, 2]) <write>
// CHECK-NEXT: 0 OUT (d, [5, 2]) <write>
// CHECK-NEXT: 1 IN (global_var, [3, 2]) <invalidation>
// CHECK-NEXT: 1 IN (i, [4, 2]) <write>
// CHECK-NEXT: 1 IN (i, [5, 0]) <write>
// CHECK-NEXT: 1 IN (d, [2, 2]) <write>
// CHECK-NEXT: 1 IN (d, [5, 2]) <write>
// CHECK-NEXT: 1 OUT (global_var, [1, 5]) <invalidation>
// CHECK-NEXT: 1 OUT (i, [1, 5]) <invalidation>
// CHECK-NEXT: 1 OUT (d, [2, 2]) <write>
// CHECK-NEXT: 1 OUT (d, [5, 2]) <write>
// CHECK-NEXT: 2 IN (global_var, [3, 2]) <invalidation>
// CHECK-NEXT: 2 IN (i, [4, 2]) <write>
// CHECK-NEXT: 2 IN (i, [5, 0]) <write>
// CHECK-NEXT: 2 IN (d, [5, 2]) <write>
// CHECK-NEXT: 2 OUT (global_var, [3, 2]) <invalidation>
// CHECK-NEXT: 2 OUT (i, [4, 2]) <write>
// CHECK-NEXT: 2 OUT (i, [5, 0]) <write>
// CHECK-NEXT: 2 OUT (d, [2, 2]) <write>
// CHECK-NEXT: 3 IN (global_var, [5, 5]) <invalidation>
// CHECK-NEXT: 3 IN (i, [4, 2]) <write>
// CHECK-NEXT: 3 IN (i, [5, 0]) <write>
// CHECK-NEXT: 3 IN (d, [5, 2]) <write>
// CHECK-NEXT: 3 OUT (global_var, [3, 2]) <invalidation>
// CHECK-NEXT: 3 OUT (i, [4, 2]) <write>
// CHECK-NEXT: 3 OUT (i, [5, 0]) <write>
// CHECK-NEXT: 3 OUT (d, [5, 2]) <write>
// CHECK-NEXT: 4 IN (global_var, [5, 5]) <invalidation>
// CHECK-NEXT: 4 IN (i, [5, 0]) <write>
// CHECK-NEXT: 4 IN (d, [5, 2]) <write>
// CHECK-NEXT: 4 OUT (global_var, [5, 5]) <invalidation>
// CHECK-NEXT: 4 OUT (i, [4, 2]) <write>
// CHECK-NEXT: 4 OUT (d, [5, 2]) <write>
// CHECK-NEXT: 5 OUT (global_var, [5, 5]) <invalidation>
// CHECK-NEXT: 5 OUT (i, [5, 0]) <write>
// CHECK-NEXT: 5 OUT (d, [5, 2]) <write>

} // namespace simple_invalidation
