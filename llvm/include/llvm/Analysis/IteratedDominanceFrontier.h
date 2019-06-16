//===- IteratedDominanceFrontier.h - Calculate IDF --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// Compute iterated dominance frontiers using a linear time algorithm.
///
/// The algorithm used here is based on:
///
///   Sreedhar and Gao. A linear time algorithm for placing phi-nodes.
///   In Proceedings of the 22nd ACM SIGPLAN-SIGACT Symposium on Principles of
///   Programming Languages
///   POPL '95. ACM, New York, NY, 62-73.
///
/// It has been modified to not explicitly use the DJ graph data structure and
/// to directly compute pruned SSA using per-variable liveness information.
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
