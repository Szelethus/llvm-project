// RUN: %clang_analyze_cc1 %s \
// RUN:   -analyzer-output=plist -o %t.plist \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=debug.ReportMacroLocations \
// RUN:   2>&1 | FileCheck %s -check-prefix=CHECK-MISSING-CONFIG

// CHECK-MISSING-CONFIG: warning: Explicitly enabled checker
// CHECK-MISSING-CONFIG-SAME: 'debug.ReportMacroLocations' creates a
// CHECK-MISSING-CONFIG-SAME: PathDiagnosticMacroPiece for each statement that
// CHECK-MISSING-CONFIG-SAME: is expanded from a macro, but the plist output
// CHECK-MISSING-CONFIG-SAME: only displays macro expansions if the analyzer
// CHECK-MISSING-CONFIG-SAME: config 'expand-macros' is set to true
//
// RUN: %clang_analyze_cc1 %s \
// RUN:   -analyzer-output=sarif -o %t.sarif \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=debug.ReportMacroLocations \
// RUN:   2>&1 | FileCheck %s -check-prefix=CHECK-CANT-EXPAND
//
// RUN: %clang_analyze_cc1 %s \
// RUN:   -analyzer-output=text \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=debug.ReportMacroLocations \
// RUN:   2>&1 | FileCheck %s -check-prefix=CHECK-CANT-EXPAND

// CHECK-CANT-EXPAND: warning: Explicitly enabled checker
// CHECK-CANT-EXPAND-SAME: 'debug.ReportMacroLocations' creates a
// CHECK-CANT-EXPAND-SAME: PathDiagnosticMacroPiece for each statement that
// CHECK-CANT-EXPAND-SAME: is expanded from a macro, but the current output
// CHECK-CANT-EXPAND-SAME: type doesn't support macro expansions
//
// HTML output expands macros by default.
//
// RUN: %clang_analyze_cc1 %s \
// RUN:   -analyzer-output=html -o %t.html \
// RUN:   -analyzer-checker=core \
// RUN:   -analyzer-checker=debug.ReportMacroLocations
