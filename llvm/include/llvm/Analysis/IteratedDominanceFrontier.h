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

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/CFGDiff.h"
#include "llvm/Support/GenericDomTree.h"
#include "llvm/Support/GenericIteratedDominanceFrontier.h"
#include <queue>

namespace llvm {

/// Determine the iterated dominance frontier, given a set of defining
/// blocks, and optionally, a set of live-in blocks.
///
/// In turn, the results can be used to place phi nodes.
///
/// This algorithm is a linear time computation of Iterated Dominance Frontiers,
/// pruned using the live-in set.
/// By default, liveness is not used to prune the IDF computation.
/// The template parameters should be of a CFG block type.
template <class NodeTy, bool IsPostDom>
class IDFCalculator : public IDFCalculatorBase<NodeTy, IsPostDom> {
public:
  using IDFCalculatorBase = typename llvm::IDFCalculatorBase<NodeTy, IsPostDom>;
  using OrderedNodeTy = typename IDFCalculatorBase::OrderedNodeTy;

  IDFCalculator(DominatorTreeBase<NodeTy, IsPostDom> &DT)
      : IDFCalculatorBase(DT) {}

  IDFCalculator(DominatorTreeBase<NodeTy, IsPostDom> &DT,
                const GraphDiff<NodeTy *, IsPostDom> *GD)
      : IDFCalculatorBase(DT), GD(GD) {}

private:
  const GraphDiff<NodeTy *, IsPostDom> *GD = nullptr;
};

class BasicBlock;

typedef IDFCalculator<BasicBlock, false> ForwardIDFCalculator;
typedef IDFCalculator<BasicBlock, true> ReverseIDFCalculator;
} // end of namespace llvm

#endif
