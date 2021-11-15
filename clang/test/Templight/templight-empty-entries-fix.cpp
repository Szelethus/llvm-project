// RUN: %clang_cc1 -templight-dump -Wno-unused-value %s 2>&1 | FileCheck %s

void a() {
  [] {};
}

// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'lambda at .*templight-empty-entries-fix.cpp:4:3'$}}
// CHECK: {{^kind:[ ]+Memoization$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^orig:[ ]+'.*templight-empty-entries-fix.cpp:4:3'$}}
// CHECK: {{^poi:[ ]+'.*templight-empty-entries-fix.cpp:4:3'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'lambda at .*templight-empty-entries-fix.cpp:4:3'$}}
// CHECK: {{^kind:[ ]+Memoization$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^orig:[ ]+'.*templight-empty-entries-fix.cpp:4:3'$}}
// CHECK: {{^poi:[ ]+'.*templight-empty-entries-fix.cpp:4:3'$}}

template <int = 0> void a() { a(); }

// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+a$}}
// CHECK: {{^kind:[ ]+DeducedTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^orig:[ ]+'.*templight-empty-entries-fix.cpp:20:25'$}}
// CHECK: {{^poi:[ ]+'.*templight-empty-entries-fix.cpp:20:31'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+template non-type parameter 0 of a$}}
// CHECK: {{^kind:[ ]+DefaultTemplateArgumentInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^orig:[ ]+'.*templight-empty-entries-fix.cpp:20:15'$}}
// CHECK: {{^poi:[ ]+'.*templight-empty-entries-fix.cpp:20:25'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+template non-type parameter 0 of a$}}
// CHECK: {{^kind:[ ]+DefaultTemplateArgumentInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^orig:[ ]+'.*templight-empty-entries-fix.cpp:20:15'$}}
// CHECK: {{^poi:[ ]+'.*templight-empty-entries-fix.cpp:20:25'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+a$}}
// CHECK: {{^kind:[ ]+DeducedTemplateArgumentSubstitution$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^orig:[ ]+'.*templight-empty-entries-fix.cpp:20:25'$}}
// CHECK: {{^poi:[ ]+'.*templight-empty-entries-fix.cpp:20:31'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'a<0>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+Begin$}}
// CHECK: {{^orig:[ ]+'.*templight-empty-entries-fix.cpp:20:25'$}}
// CHECK: {{^poi:[ ]+'.*templight-empty-entries-fix.cpp:20:31'$}}
// CHECK-LABEL: {{^---$}}
// CHECK: {{^name:[ ]+'a<0>'$}}
// CHECK: {{^kind:[ ]+TemplateInstantiation$}}
// CHECK: {{^event:[ ]+End$}}
// CHECK: {{^orig:[ ]+'.*templight-empty-entries-fix.cpp:20:25'$}}
// CHECK: {{^poi:[ ]+'.*templight-empty-entries-fix.cpp:20:31'$}}

template <int> struct b { typedef int c; };
template <bool d = true, class = typename b<d>::c> void a() { a(); }
