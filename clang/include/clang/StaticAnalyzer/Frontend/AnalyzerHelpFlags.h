//===-- CheckerRegistration.h - Checker Registration Function ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_FRONTEND_CHECKERREGISTRATION_H
#define LLVM_CLANG_STATICANALYZER_FRONTEND_CHECKERREGISTRATION_H


#include "clang/Frontend/CompilerInstance.h"
namespace llvm {
class raw_ostream;
} // namespace llvm

namespace clang {

class CompilerInstance;

namespace ento {

void printCheckerHelp(raw_ostream &OS, CompilerInstance &CI);
void printEnabledCheckerList(raw_ostream &OS, CompilerInstance &CI);
void printAnalyzerConfigList(raw_ostream &OS);
void printCheckerConfigList(raw_ostream &OS, CompilerInstance &CI);

} // namespace ento
} // namespace clang

#endif
