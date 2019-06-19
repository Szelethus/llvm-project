//===- IteratedDominanceFrontier.h - Calculate IDF --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_IDF_H
#define LLVM_ANALYSIS_IDF_H

#include "llvm/IR/CFGDiff.h"
#include "llvm/Support/GenericIteratedDominanceFrontier.h"

namespace llvm {

class BasicBlock;

template <bool IsPostDom>
class IDFCalculator final : public IDFCalculatorBase<BasicBlock, IsPostDom> {
public:
  using IDFCalculatorBase =
      typename llvm::IDFCalculatorBase<BasicBlock, IsPostDom>;

  IDFCalculator(DominatorTreeBase<BasicBlock, IsPostDom> &DT)
      : IDFCalculatorBase(DT) {}

  IDFCalculator(DominatorTreeBase<BasicBlock, IsPostDom> &DT,
                const GraphDiff<BasicBlock *, IsPostDom> *GD)
      : IDFCalculatorBase(
            DT,
            [GD](const NodeRef &BB) {
              using SnapShotBBPair =
                  std::pair<const GraphDiff<BasicBlock *, IsPostDom> *,
                            OrderedNodeTy>;

              ChildrenTy Ret;
              for (auto Pair : children<SnapShotBBPair>({GD, BB}))
                Ret.emplace_back(Pair.second);
              return Ret;
            }),
        GD(GD) {
    assert(GD);
  }

  using NodeRef = BasicBlock *;
  using ChildrenTy = typename IDFCalculatorBase::ChildrenTy;
  using OrderedNodeTy = typename IDFCalculatorBase::OrderedNodeTy;

private:
  const GraphDiff<BasicBlock *, IsPostDom> *GD = nullptr;
};

using ForwardIDFCalculator = IDFCalculator<false>;
using ReverseIDFCalculator = IDFCalculator<true>;

} // end of namespace llvm

#endif
