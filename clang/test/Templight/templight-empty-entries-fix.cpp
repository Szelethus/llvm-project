// RUN: %clang_cc1 -templight-dump -Wno-unused-value %s 2>&1 | FileCheck %s

void a() {
  [] {};
}

// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'\(lambda at .*templight-empty-entries-fix.cpp:4:3\)'$}}
// CHECK: {{^kind:[ ]+Memoization$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^orig:[ ]+'.*templight-empty-entries-fix.cpp:4:3'$}}
// CHECK: {{^poi:[ ]+'.*templight-empty-entries-fix.cpp:4:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'\(lambda at .*templight-empty-entries-fix.cpp:4:3\)'$}}
// CHECK: {{^kind:[ ]+Memoization$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^orig:[ ]+'.*templight-empty-entries-fix.cpp:4:3'$}}
// CHECK: {{^poi:[ ]+'.*templight-empty-entries-fix.cpp:4:3'$}}
