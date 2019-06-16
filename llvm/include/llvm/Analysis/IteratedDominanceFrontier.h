//===- IteratedDominanceFrontier.h - Calculate IDF --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_IDF_H
#define LLVM_ANALYSIS_IDF_H

#include "llvm/Support/GenericIteratedDominanceFrontier.h"

namespace llvm {

class BasicBlock;

using ForwardIDFCalculator = IDFCalculatorBase<BasicBlock, false>;
using ReverseIDFCalculator = IDFCalculatorBase<BasicBlock, true>;

} // end of namespace llvm

#endif
