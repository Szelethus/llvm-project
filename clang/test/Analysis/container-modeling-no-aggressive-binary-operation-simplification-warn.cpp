// RUN: %clang_analyze_cc1 \
// RUN: -analyzer-checker=core,cplusplus,alpha.cplusplus.ContainerModeling \
// RUN: %s 2>&1 | FileCheck %s

// Check if the return value is 0.
// RUN: %clang_analyze_cc1 \
// RUN: -analyzer-checker=core,cplusplus,alpha.cplusplus.ContainerModeling

// CHECK: warning: alpha.cplusplus.ContainerModeling cannot be enabled with
// CHECK-SAME: analyzer option 'aggressive-binary-operation-simplification'
// CHECK-SAME: == false
