// RUN: %clang_analyze_cc1 %s \
// RUN:   -analyzer-checker=debug.DumpCFG \
// RUN:   -analyzer-checker=debug.DumpGenSets \
// RUN:   -analyzer-checker=debug.DumpKillSets \
// RUN:   -analyzer-checker=debug.DumpReachingDefinitions \
// RUN:   2>&1 | FileCheck %s

int global_var;

int getInt();
int *getIntPtr();
bool coin();

void single_vardecl_in_declstmt() {
  int *ptr = getIntPtr();
}
// [B2 (ENTRY)] -> [B1] -> [B0 (EXIT)]

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 1 (ptr, [1, 3]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 IN (ptr, [1, 3]) <write>
// CHECK-NEXT: 0 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 OUT (ptr, [1, 3]) <write>
// CHECK-NEXT: 1 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 1 OUT (ptr, [1, 3]) <write>

void multiple_vardecl_in_declstmt() {
  int *ptr = getIntPtr(), i;
}
// [B2 (ENTRY)] -> [B1] -> [B0 (EXIT)]

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 1 (ptr, [1, 3]) <write>
// CHECK-NEXT: 1 (i, [1, 4]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 IN (ptr, [1, 3]) <write>
// CHECK-NEXT: 0 IN (i, [1, 4]) <write>
// CHECK-NEXT: 0 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 OUT (ptr, [1, 3]) <write>
// CHECK-NEXT: 0 OUT (i, [1, 4]) <write>
// CHECK-NEXT: 1 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 1 OUT (ptr, [1, 3]) <write>
// CHECK-NEXT: 1 OUT (i, [1, 4]) <write>

void function_and_vardecl_in_declstmt() {
  int *ptr = getIntPtr(), a();
}
// [B2 (ENTRY)] -> [B1] -> [B0 (EXIT)]

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 1 (ptr, [1, 3]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 IN (ptr, [1, 3]) <write>
// CHECK-NEXT: 0 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 OUT (ptr, [1, 3]) <write>
// CHECK-NEXT: 1 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 1 OUT (ptr, [1, 3]) <write>

void single_def_in_same_block() {
  int *ptr = getIntPtr();

  if (coin())
    ptr = 0;

  if (!ptr)
    *ptr = 5;
}
//                    -> [B3] ->    -> [B1] ->
//                   /          \  /          \
// [B5 (ENTRY)] -> [B4] ------> [B2] ---> [B0 (EXIT)]

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 3 (ptr, [3, 3]) <write>
// CHECK-NEXT: 4 (global_var, [4, 6]) <invalidation>
// CHECK-NEXT: 4 (ptr, [4, 3]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 3 (ptr, [4, 3]) <write>
// CHECK-NEXT: 4 (ptr, [3, 3]) <write>
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (global_var, [4, 6]) <invalidation>
// CHECK-NEXT: 0 IN (ptr, [3, 3]) <write>
// CHECK-NEXT: 0 IN (ptr, [4, 3]) <write>
// CHECK-NEXT: 0 OUT (global_var, [4, 6]) <invalidation>
// CHECK-NEXT: 0 OUT (ptr, [3, 3]) <write>
// CHECK-NEXT: 0 OUT (ptr, [4, 3]) <write>
// CHECK-NEXT: 1 IN (global_var, [4, 6]) <invalidation>
// CHECK-NEXT: 1 IN (ptr, [3, 3]) <write>
// CHECK-NEXT: 1 IN (ptr, [4, 3]) <write>
// CHECK-NEXT: 1 OUT (global_var, [4, 6]) <invalidation>
// CHECK-NEXT: 1 OUT (ptr, [3, 3]) <write>
// CHECK-NEXT: 1 OUT (ptr, [4, 3]) <write>
// CHECK-NEXT: 2 IN (global_var, [4, 6]) <invalidation>
// CHECK-NEXT: 2 IN (ptr, [3, 3]) <write>
// CHECK-NEXT: 2 IN (ptr, [4, 3]) <write>
// CHECK-NEXT: 2 OUT (global_var, [4, 6]) <invalidation>
// CHECK-NEXT: 2 OUT (ptr, [3, 3]) <write>
// CHECK-NEXT: 2 OUT (ptr, [4, 3]) <write>
// CHECK-NEXT: 3 IN (global_var, [4, 6]) <invalidation>
// CHECK-NEXT: 3 IN (ptr, [4, 3]) <write>
// CHECK-NEXT: 3 OUT (global_var, [4, 6]) <invalidation>
// CHECK-NEXT: 3 OUT (ptr, [3, 3]) <write>
// CHECK-NEXT: 4 OUT (global_var, [4, 6]) <invalidation>
// CHECK-NEXT: 4 OUT (ptr, [4, 3]) <write>

void different_assignments() {
  int i = getInt();

  if (coin())
    i = 0;

  i += 3;

  if (!coin())
    i -= 2;

  i *= 9;

  if (i = 0)
    ;
}
//                    -> [B5] ->    -> [B3] ->    -> [B1] ->
//                   /          \  /          \  /          \
// [B7 (ENTRY)] -> [B6] ------> [B4] -------> [B2] ---> [B0 (EXIT)]

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 2 (i, [2, 5]) <write>
// CHECK-NEXT: 3 (i, [3, 2]) <write>
// CHECK-NEXT: 4 (global_var, [4, 5]) <invalidation>
// CHECK-NEXT: 4 (i, [4, 2]) <write>
// CHECK-NEXT: 5 (i, [5, 2]) <write>
// CHECK-NEXT: 6 (global_var, [6, 6]) <invalidation>
// CHECK-NEXT: 6 (i, [6, 3]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 2 (i, [3, 2]) <write>
// CHECK-NEXT: 2 (i, [4, 2]) <write>
// CHECK-NEXT: 2 (i, [5, 2]) <write>
// CHECK-NEXT: 2 (i, [6, 3]) <write>
// CHECK-NEXT: 3 (i, [2, 5]) <write>
// CHECK-NEXT: 3 (i, [4, 2]) <write>
// CHECK-NEXT: 3 (i, [5, 2]) <write>
// CHECK-NEXT: 3 (i, [6, 3]) <write>
// CHECK-NEXT: 4 (global_var, [6, 6]) <invalidation>
// CHECK-NEXT: 4 (i, [2, 5]) <write>
// CHECK-NEXT: 4 (i, [3, 2]) <write>
// CHECK-NEXT: 4 (i, [5, 2]) <write>
// CHECK-NEXT: 4 (i, [6, 3]) <write>
// CHECK-NEXT: 5 (i, [2, 5]) <write>
// CHECK-NEXT: 5 (i, [3, 2]) <write>
// CHECK-NEXT: 5 (i, [4, 2]) <write>
// CHECK-NEXT: 5 (i, [6, 3]) <write>
// CHECK-NEXT: 6 (global_var, [4, 5]) <invalidation>
// CHECK-NEXT: 6 (i, [2, 5]) <write>
// CHECK-NEXT: 6 (i, [3, 2]) <write>
// CHECK-NEXT: 6 (i, [4, 2]) <write>
// CHECK-NEXT: 6 (i, [5, 2]) <write>
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (global_var, [4, 5]) <invalidation>
// CHECK-NEXT: 0 IN (i, [2, 5]) <write>
// CHECK-NEXT: 0 OUT (global_var, [4, 5]) <invalidation>
// CHECK-NEXT: 0 OUT (i, [2, 5]) <write>
// CHECK-NEXT: 1 IN (global_var, [4, 5]) <invalidation>
// CHECK-NEXT: 1 IN (i, [2, 5]) <write>
// CHECK-NEXT: 1 OUT (global_var, [4, 5]) <invalidation>
// CHECK-NEXT: 1 OUT (i, [2, 5]) <write>
// CHECK-NEXT: 2 IN (global_var, [4, 5]) <invalidation>
// CHECK-NEXT: 2 IN (i, [3, 2]) <write>
// CHECK-NEXT: 2 IN (i, [4, 2]) <write>
// CHECK-NEXT: 2 OUT (global_var, [4, 5]) <invalidation>
// CHECK-NEXT: 2 OUT (i, [2, 5]) <write>
// CHECK-NEXT: 3 IN (global_var, [4, 5]) <invalidation>
// CHECK-NEXT: 3 IN (i, [4, 2]) <write>
// CHECK-NEXT: 3 OUT (global_var, [4, 5]) <invalidation>
// CHECK-NEXT: 3 OUT (i, [3, 2]) <write>
// CHECK-NEXT: 4 IN (global_var, [6, 6]) <invalidation>
// CHECK-NEXT: 4 IN (i, [5, 2]) <write>
// CHECK-NEXT: 4 IN (i, [6, 3]) <write>
// CHECK-NEXT: 4 OUT (global_var, [4, 5]) <invalidation>
// CHECK-NEXT: 4 OUT (i, [4, 2]) <write>
// CHECK-NEXT: 5 IN (global_var, [6, 6]) <invalidation>
// CHECK-NEXT: 5 IN (i, [6, 3]) <write>
// CHECK-NEXT: 5 OUT (global_var, [6, 6]) <invalidation>
// CHECK-NEXT: 5 OUT (i, [5, 2]) <write>
// CHECK-NEXT: 6 OUT (global_var, [6, 6]) <invalidation>
// CHECK-NEXT: 6 OUT (i, [6, 3]) <write>

namespace example_1 {

int flag;

void foo() {
  flag = coin();
}
// [B2 (ENTRY)] -> [B1] -> [B0 (EXIT)]

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 1 (flag, [1, 5]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 IN (flag, [1, 5]) <write>
// CHECK-NEXT: 0 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 OUT (flag, [1, 5]) <write>
// CHECK-NEXT: 1 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 1 OUT (flag, [1, 5]) <write>

void f() {
  int *x = nullptr;
  flag = 1;

  foo();
  if (flag)
    x = new int;

  foo();
  if (flag)
    *x = 5;
}
//                    -> [B3] ->    -> [B1] ->
//                   /          \  /          \
// [B5 (ENTRY)] -> [B4] ------> [B2] ---> [B0 (EXIT)]

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 2 (global_var, [2, 2]) <invalidation>
// CHECK-NEXT: 2 (flag, [2, 2]) <invalidation>
// CHECK-NEXT: 3 (x, [3, 3]) <write>
// CHECK-NEXT: 4 (global_var, [4, 8]) <invalidation>
// CHECK-NEXT: 4 (flag, [4, 8]) <invalidation>
// CHECK-NEXT: 4 (x, [4, 2]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 2 (global_var, [4, 8]) <invalidation>
// CHECK-NEXT: 2 (flag, [4, 8]) <invalidation>
// CHECK-NEXT: 3 (x, [4, 2]) <write>
// CHECK-NEXT: 4 (global_var, [2, 2]) <invalidation>
// CHECK-NEXT: 4 (flag, [2, 2]) <invalidation>
// CHECK-NEXT: 4 (x, [3, 3]) <write>
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (global_var, [2, 2]) <invalidation>
// CHECK-NEXT: 0 IN (flag, [2, 2]) <invalidation>
// CHECK-NEXT: 0 IN (x, [3, 3]) <write>
// CHECK-NEXT: 0 IN (x, [4, 2]) <write>
// CHECK-NEXT: 0 OUT (global_var, [2, 2]) <invalidation>
// CHECK-NEXT: 0 OUT (flag, [2, 2]) <invalidation>
// CHECK-NEXT: 0 OUT (x, [3, 3]) <write>
// CHECK-NEXT: 0 OUT (x, [4, 2]) <write>
// CHECK-NEXT: 1 IN (global_var, [2, 2]) <invalidation>
// CHECK-NEXT: 1 IN (flag, [2, 2]) <invalidation>
// CHECK-NEXT: 1 IN (x, [3, 3]) <write>
// CHECK-NEXT: 1 IN (x, [4, 2]) <write>
// CHECK-NEXT: 1 OUT (global_var, [2, 2]) <invalidation>
// CHECK-NEXT: 1 OUT (flag, [2, 2]) <invalidation>
// CHECK-NEXT: 1 OUT (x, [3, 3]) <write>
// CHECK-NEXT: 1 OUT (x, [4, 2]) <write>
// CHECK-NEXT: 2 IN (global_var, [4, 8]) <invalidation>
// CHECK-NEXT: 2 IN (flag, [4, 8]) <invalidation>
// CHECK-NEXT: 2 IN (x, [3, 3]) <write>
// CHECK-NEXT: 2 IN (x, [4, 2]) <write>
// CHECK-NEXT: 2 OUT (global_var, [2, 2]) <invalidation>
// CHECK-NEXT: 2 OUT (flag, [2, 2]) <invalidation>
// CHECK-NEXT: 2 OUT (x, [3, 3]) <write>
// CHECK-NEXT: 2 OUT (x, [4, 2]) <write>
// CHECK-NEXT: 3 IN (global_var, [4, 8]) <invalidation>
// CHECK-NEXT: 3 IN (flag, [4, 8]) <invalidation>
// CHECK-NEXT: 3 IN (x, [4, 2]) <write>
// CHECK-NEXT: 3 OUT (global_var, [4, 8]) <invalidation>
// CHECK-NEXT: 3 OUT (flag, [4, 8]) <invalidation>
// CHECK-NEXT: 3 OUT (x, [3, 3]) <write>
// CHECK-NEXT: 4 OUT (global_var, [4, 8]) <invalidation>
// CHECK-NEXT: 4 OUT (flag, [4, 8]) <invalidation>
// CHECK-NEXT: 4 OUT (x, [4, 2]) <write>

} // end of namespace example_1

void assignment_buried_beneath_parentheses() {
  int *ptr = getIntPtr();
  if (coin())
    (((((((((((((((ptr))))))) = getIntPtr()))))))));
}
//                    -> [B1] ->
//                   /          \
// [B3 (ENTRY)] -> [B2] ---> [B0 (EXIT)]

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 2 (global_var, [2, 6]) <invalidation>
// CHECK-NEXT: 2 (ptr, [2, 3]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (global_var, [2, 6]) <invalidation>
// CHECK-NEXT: 2 (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 IN (global_var, [2, 6]) <invalidation>
// CHECK-NEXT: 0 IN (ptr, [2, 3]) <write>
// CHECK-NEXT: 0 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 OUT (global_var, [2, 6]) <invalidation>
// CHECK-NEXT: 0 OUT (ptr, [2, 3]) <write>
// CHECK-NEXT: 1 IN (global_var, [2, 6]) <invalidation>
// CHECK-NEXT: 1 IN (ptr, [2, 3]) <write>
// CHECK-NEXT: 1 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 1 OUT (ptr, [2, 3]) <write>
// CHECK-NEXT: 2 OUT (global_var, [2, 6]) <invalidation>
// CHECK-NEXT: 2 OUT (ptr, [2, 3]) <write>

void single_parameter(int i) {
  int *ptr = getIntPtr();
}
// [B2 (ENTRY)] -> [B1] -> [B0 (EXIT)]

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 1 (ptr, [1, 3]) <write>
// CHECK-NEXT: 2 (i, [2, 0]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 IN (i, [2, 0]) <write>
// CHECK-NEXT: 0 IN (ptr, [1, 3]) <write>
// CHECK-NEXT: 0 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 OUT (i, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (ptr, [1, 3]) <write>
// CHECK-NEXT: 1 IN (i, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 1 OUT (i, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (ptr, [1, 3]) <write>
// CHECK-NEXT: 2 OUT (i, [2, 0]) <write>

void multiple_parameters(int i, int j, int k) {
  int *ptr = getIntPtr();
}
// [B2 (ENTRY)] -> [B1] -> [B0 (EXIT)]

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 1 (ptr, [1, 3]) <write>
// CHECK-NEXT: 2 (i, [2, 0]) <write>
// CHECK-NEXT: 2 (j, [2, 0]) <write>
// CHECK-NEXT: 2 (k, [2, 0]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 IN (i, [2, 0]) <write>
// CHECK-NEXT: 0 IN (j, [2, 0]) <write>
// CHECK-NEXT: 0 IN (k, [2, 0]) <write>
// CHECK-NEXT: 0 IN (ptr, [1, 3]) <write>
// CHECK-NEXT: 0 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 0 OUT (i, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (j, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (k, [2, 0]) <write>
// CHECK-NEXT: 0 OUT (ptr, [1, 3]) <write>
// CHECK-NEXT: 1 IN (i, [2, 0]) <write>
// CHECK-NEXT: 1 IN (j, [2, 0]) <write>
// CHECK-NEXT: 1 IN (k, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (global_var, [1, 2]) <invalidation>
// CHECK-NEXT: 1 OUT (i, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (j, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (k, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (ptr, [1, 3]) <write>
// CHECK-NEXT: 2 OUT (i, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (j, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (k, [2, 0]) <write>

void assignment_and_declaration_on_same_line(int i, int j) {
  int k = i = j = 0;
}
// [B2 (ENTRY)] -> [B1] -> [B0 (EXIT)]

// CHECK:      GEN sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (i, [1, 5]) <write>
// CHECK-NEXT: 1 (j, [1, 2]) <write>
// CHECK-NEXT: 1 (k, [1, 7]) <write>
// CHECK-NEXT: 2 (i, [2, 0]) <write>
// CHECK-NEXT: 2 (j, [2, 0]) <write>
// CHECK-NEXT: KILL sets: blockid (varname [blockid, elementid])
// CHECK-NEXT: 1 (i, [2, 0]) <write>
// CHECK-NEXT: 1 (j, [2, 0]) <write>
// CHECK-NEXT: 2 (i, [1, 5]) <write>
// CHECK-NEXT: 2 (j, [1, 2]) <write>
// CHECK-NEXT: Reaching definition sets: blockid IN/OUT (varname [blockid, elementid])
// CHECK-NEXT: 0 IN (i, [1, 5]) <write>
// CHECK-NEXT: 0 IN (j, [1, 2]) <write>
// CHECK-NEXT: 0 IN (k, [1, 7]) <write>
// CHECK-NEXT: 0 OUT (i, [1, 5]) <write>
// CHECK-NEXT: 0 OUT (j, [1, 2]) <write>
// CHECK-NEXT: 0 OUT (k, [1, 7]) <write>
// CHECK-NEXT: 1 IN (i, [2, 0]) <write>
// CHECK-NEXT: 1 IN (j, [2, 0]) <write>
// CHECK-NEXT: 1 OUT (i, [1, 5]) <write>
// CHECK-NEXT: 1 OUT (j, [1, 2]) <write>
// CHECK-NEXT: 1 OUT (k, [1, 7]) <write>
// CHECK-NEXT: 2 OUT (i, [2, 0]) <write>
// CHECK-NEXT: 2 OUT (j, [2, 0]) <write>

