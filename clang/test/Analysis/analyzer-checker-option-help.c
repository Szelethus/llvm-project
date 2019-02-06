// RUN: %clang_cc1 -analyzer-checker-option-help 2>&1 | FileCheck %s

// CHECK: OVERVIEW: Clang Static Analyzer Checker and Package Option List
//
// CHECK: USAGE: clang -cc1 [CLANG_OPTIONS] -analyzer-config <OPTION1=VALUE,OPTION2=VALUE,...>
//
// CHECK:        clang -cc1 [CLANG_OPTIONS] -analyzer-config OPTION1=VALUE,
// CHECK-SAME:     -analyzer-config OPTION2=VALUE, ...
//
// CHECK:        clang [CLANG_OPTIONS] -Xclang -analyzer-config
// CHECK-SAME:     -Xclang<OPTION1=VALUE,OPTION2=VALUE,...>
//
// CHECK:        clang [CLANG_OPTIONS] -Xclang -analyzer-config
// CHECK-SAME:     -Xclang OPTION1=VALUE, -Xclang -analyzer-config
// CHECK-SAME:     -Xclang OPTION2=VALUE, ...
//
// CHECK: OPTIONS:
//
// CHECK:   alpha.clone.CloneChecker:MinimumCloneComplexity  
// CHECK-SAME:   (int) Ensures that every clone has at least
// CHECK:        the given complexity. Complexity is here
// CHECK:        defined as the total amount of children
// CHECK:        of a statement. This constraint assumes
// CHECK:        the first statement in the group is representative
// CHECK:        for all other statements in the group in
// CHECK:        terms of complexity. (default: 50)
