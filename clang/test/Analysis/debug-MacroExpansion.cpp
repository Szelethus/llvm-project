// RUN: not %clang_analyze_cc1 %s \
// RUN:   -analyzer-output=plist -o %t.plist \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=debug.ReportMacroLocations \
// RUN:   2>&1 | FileCheck %s -check-prefix=CHECK-MISSING-CONFIG

// CHECK-MISSING-CONFIG: error: Explicitly enabled checker
// CHECK-MISSING-CONFIG-SAME: 'debug.ReportMacroLocations' creates a
// CHECK-MISSING-CONFIG-SAME: PathDiagnosticMacroPiece for each statement that
// CHECK-MISSING-CONFIG-SAME: is expanded from a macro, but the plist output
// CHECK-MISSING-CONFIG-SAME: only displays macro expansions if the analyzer
// CHECK-MISSING-CONFIG-SAME: config 'expand-macros' is set to true


// RUN: not %clang_analyze_cc1 %s \
// RUN:   -analyzer-output=sarif -o %t.sarif \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=debug.ReportMacroLocations \
// RUN:   2>&1 | FileCheck %s -check-prefix=CHECK-CANT-EXPAND
//
// RUN: not %clang_analyze_cc1 %s \
// RUN:   -analyzer-output=text \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=debug.ReportMacroLocations \
// RUN:   2>&1 | FileCheck %s -check-prefix=CHECK-CANT-EXPAND

// CHECK-CANT-EXPAND: error: Explicitly enabled checker
// CHECK-CANT-EXPAND-SAME: 'debug.ReportMacroLocations' creates a
// CHECK-CANT-EXPAND-SAME: PathDiagnosticMacroPiece for each statement that
// CHECK-CANT-EXPAND-SAME: is expanded from a macro, but the current output
// CHECK-CANT-EXPAND-SAME: type doesn't support macro expansions; use plist or
// CHECK-CANT-EXPAND-SAME: html


// HTML output expands macros by default.
//
// RUN: rm -rf %t.html
//
// RUN: %clang_analyze_cc1 -verify %s \
// RUN:   -analyzer-output=html -o %t.html \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=debug.ReportMacroLocations
//
// RUN: rm -rf %t.plist
//
// RUN: %clang_analyze_cc1 -verify %s \
// RUN:   -analyzer-output=plist -o %t.plist \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=debug.ReportMacroLocations \
// RUN:   -analyzer-config expand-macros=true

void print(const void *);

#define PRINT(x) \
  print(x);

void f() {
  PRINT("Remember the vasa!"); // expected-warning{{expanded from a macro}}
}

#define TRICKY_MACRO_EXPANSION(a, b, c, ...) \
  print(__VA_ARGS__);

void g() {
  TRICKY_MACRO_EXPANSION(,,, "How much wood could a woodchuck chuck?");
  // expected-warning@-1{{expanded from a macro}}
}
