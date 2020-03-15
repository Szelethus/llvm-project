//===--- ReachingDefinitions.h ----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Calculates reaching definitions for a CFG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_ANALYSIS_ANALYSES_REACHABLE_DEFINITIONS_H
#define LLVM_CLANG_ANALYSIS_ANALYSES_REACHABLE_DEFINITIONS_H

#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Analysis/AnalysisDeclContext.h"
#include "clang/Analysis/CFG.h"
#include "llvm/ADT/SmallVector.h"
#include <set>

namespace clang {

//===----------------------------------------------------------------------===//
// Since reaching definitions was implemented for instructions, it isn't well
// thought out what a definition of a variable is in C/C++, nor what we really
// mean under the term variable.
//===----------------------------------------------------------------------===//

namespace ReachingDefinitionsDetail {

//===----------------------------------------------------------------------===//
// What a 'variable' is:
// We define a variable as a VarDecl, and optionally, a non-empty list of
// FieldDecls to express parts of a struct. This implies that we regard the
// fields of a VarDecl as 'variables'.
// TODO: How do we describe the implicit this parameter for methods?
//===----------------------------------------------------------------------===//

using FieldChainTy = llvm::SmallVector<const FieldDecl *, 2>;
bool operator<(const FieldChainTy &Lhs, const FieldChainTy &Rhs);

struct Variable {
  const VarDecl *Var;
  FieldChainTy FieldChain;

  Variable(const VarDecl *Var, FieldChainTy FieldChain)
      : Var(Var), FieldChain(FieldChain) {}
};

struct VariableLess {
  // TODO: const ref? but what if we change this to immutablelist?
  bool operator()(Variable Lhs, Variable Rhs) {
    return std::make_pair(Lhs.Var, Lhs.FieldChain) <
           std::make_pair(Rhs.Var, Rhs.FieldChain);
  }
};

//===----------------------------------------------------------------------===//
// What a 'definition of a variable' is:
// Whatever statement that writes a 'variable', or may write a 'variable'.
// We note whether know that the definition is a write, or just could be a
// write, as well as the CFGElement that contains the defining statement.
//===----------------------------------------------------------------------===//

class Definition : public Variable {
public:
  enum DefinitionKind { Write, Invalidation };

  CFGBlock::ConstCFGElementRef E;
  DefinitionKind Kind;

public:
  Definition(const VarDecl *Var, FieldChainTy FieldChain,
             CFGBlock::ConstCFGElementRef E, DefinitionKind Kind)
      : Variable(Var, std::move(FieldChain)), E(E), Kind(Kind) {}

  Definition(const VarDecl *Var, CFGBlock::ConstCFGElementRef E,
             DefinitionKind Kind)
      : Definition(Var, /*FieldChain=*/{}, E, Kind) {}

  const CFGBlock *getCFGBlock() const { return E.getParent(); }

  // (varname [blockid, elementid]) (reason)
  void dump() const;
};

struct VarAndCFGElementLess {
  bool operator()(Definition Lhs, Definition Rhs) const {
    return std::tie(Lhs.Var, Lhs.FieldChain, Lhs.E) <
           std::tie(Rhs.Var, Rhs.FieldChain, Rhs.E);
  }
};

/// A set of definitions sorted only by the variable, so that each basic block
/// may only emit a single definition for any single variable.
// TODO: We need to track more then a single definition to a variable for a
// block's GEN set. Say, the static analyzer manages to prove that a potential
// invalidation definition (like a function call) didn't write the variable, we
// need to retrieve the definitions up to that point in the block.
using GenSet = std::set<Definition, VariableLess>;

/// A set of definitions sorted by the variable and the location of the
/// definition. For KILL, IN and OUT sets this is correct, because a CFGBlock
/// may kill several definitions of the same variables from different locations.
using DefinitionSet = std::set<Definition, VarAndCFGElementLess>;

//===----------------------------------------------------------------------===//
// Determining whether a statement modifies a variable is a challanging, and
// requires using expensive-to-create matchers, and an easily extensible
// interface, hence the use of a builder class outside of the main calculator.
//===----------------------------------------------------------------------===//

class GenSetBuilder;

class GenSetMatcherCallback : public ast_matchers::MatchFinder::MatchCallback {
protected:
  GenSetBuilder &GSBuilder;

  GenSetMatcherCallback(GenSetBuilder &GSBuilder) : GSBuilder(GSBuilder) {}
};

/// Responsible for building the GEN sets for each basic block.
///
/// Since pointer escapes or function calls in general require us to generate
/// definitions that are invalidations, we need to gather all variables relevant
/// for this analysis, like parameters, locals and globals. We refer to this
/// stage as 'variable finding':
///   * Collect all non-local, visible variables
///   * Collect all local variables within the function that is used
///
/// The actual building of GEN sets has two stages:
///   1. For each CFGStmt in the CFGBlock, look for a statement that may be a
///      definition of an expression. ('definition finding')
///   2. Search expressions for variables recursively. ('expression finding')
class GenSetBuilder {
  using DefinitionKind = Definition::DefinitionKind;

  // Fields that shouldn't change after the construction of the builder object.

  llvm::SmallVector<std::unique_ptr<GenSetMatcherCallback>, 8> Callbacks;

  ast_matchers::MatchFinder VariableFinder;
  ast_matchers::MatchFinder DefinitionFinder;
  ast_matchers::MatchFinder ExpressionFinder;

  const Decl *D;
  ASTContext *Context = nullptr;

  std::set<Variable, VariableLess> AllVariables;
  std::set<Variable, VariableLess> AllGlobalVariables;

  // Fields that are changed at and during each GEN set construction.

  GenSet *CurrentGenSet;
  Optional<CFGBlock::ConstCFGElementRef> CurrentCFGElem;
  DefinitionKind CurrentKind = Definition::Write;

private:
  // TODO: Make this public and allow custom matchers to be added?
  template <ast_matchers::MatchFinder GenSetBuilder::*Finder,
            class GenSetMatcherCallbackTy>
  void addMatcher() {
    Callbacks.emplace_back(std::make_unique<GenSetMatcherCallbackTy>(*this));
    (this->*Finder)
        .addMatcher(GenSetMatcherCallbackTy::getMatcher(),
                    Callbacks.back().get());
  }

public:
  GenSetBuilder(const Decl *D);

  //===--------------------------------------------------------------------===//
  // Methods for retrieving a GEN set.
  //===--------------------------------------------------------------------===//
  void getGenSetForCFGBlock(const CFGBlock *B, GenSet &Gen);

  void getGenSetForParameters(const CFGBlock *Entry, GenSet &Gen);

  //===--------------------------------------------------------------------===//
  // Utility methods for building a GEN set. These are public because the
  // callback objects will need to call them.
  //===--------------------------------------------------------------------===//

  /// When we find a definition to an *expression*, we need to see if that
  /// expression is a variable, or some other expression that needs further
  /// processing, like in this case:
  ///   (a, b) = 10;
  void handleExpr(const Expr *E, DefinitionKind Kind);

  /// Insert a new defintion of a variable into the current GEN set.
  void insertToGenSet(const VarDecl *Var, FieldChainTy FieldChain,
                      DefinitionKind Kind);

  void insertToGenSet(const VarDecl *Var, DefinitionKind Kind) {
    insertToGenSet(Var, FieldChainTy{}, Kind);
  }

  void insertToGenSet(const VarDecl *Var, FieldChainTy FieldChain) {
    insertToGenSet(Var, FieldChain, CurrentKind);
  }

  void insertToGenSet(const VarDecl *Var) {
    insertToGenSet(Var, FieldChainTy{});
  }

  void invalidateGlobals() {
    for (const Variable &Var : AllGlobalVariables)
      insertToGenSet(Var.Var, Var.FieldChain, DefinitionKind::Invalidation);
  }

  /// Called during the initialization of the builder to collect all variables
  /// in a function.
  void addVariable(const VarDecl *Var, FieldChainTy FieldChain);
};

} // end of namespace ReachingDefinitionsDetail

/// Calculates reaching definitions for each basic block. This calculation
/// doesn't try to argue about aliasing, meaning that some definitions are
/// definite write (we know that the variable it written), and some are a result
/// of invalidation, like passing a variable as a non-const reference to a
/// function.
class ReachingDefinitionsCalculator : public ManagedAnalysis {
  using GenSetBuilder = ReachingDefinitionsDetail::GenSetBuilder;

public:
  using GenSet = ReachingDefinitionsDetail::GenSet;
  using DefinitionKind = ReachingDefinitionsDetail::Definition::DefinitionKind;
  using DefinitionSet = ReachingDefinitionsDetail::DefinitionSet;

  void calculate();

  void dumpKillSet() const;
  void dumpGenSet() const;
  void dumpReachingDefinitions() const;

  static ReachingDefinitionsCalculator *create(AnalysisDeclContext &Ctx) {
    return new ReachingDefinitionsCalculator(Ctx.getDecl(), Ctx.getCFG());
  }

  static const void *getTag() {
    static int x;
    return &x;
  }

private:
  ReachingDefinitionsCalculator(const Decl *D, const CFG *cfg);

  void init();

  const CFG *cfg;
  GenSetBuilder GSBuilder;

  // TODO: Make the GEN set public to allow clients to remove definitions, or
  // possibly mark an invalidation as write.
  std::map<const CFGBlock *, GenSet> Gen;
  std::map<const CFGBlock *, DefinitionSet> Kill;

public:
  std::map<const CFGBlock *, DefinitionSet> In;
  std::map<const CFGBlock *, DefinitionSet> Out;
};

} // end of namespace clang

#endif // LLVM_CLANG_ANALYSIS_ANALYSES_REACHABLE_DEFINITIONS_H
