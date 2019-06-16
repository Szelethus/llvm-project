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

namespace simple_invalidation {

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
