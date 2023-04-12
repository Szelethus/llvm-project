//===- VariantLikeTypeModeling.h ---------------------------------*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//  TODO
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_STATICANALYZER_CHECKER_VARIANTLIKETYPEMODELING_H
#define LLVM_CLANG_LIB_STATICANALYZER_CHECKER_VARIANTLIKETYPEMODELING_H

#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallDescription.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "llvm/ADT/FoldingSet.h"

namespace clang {
namespace ento {
namespace variant_modeling {

const TemplateArgument& getFirstTemplateArgument(const CallEvent &Call);
bool isObjectOf(QualType t, QualType to);
bool isCopyConstructorCallEvent (const CallEvent& Call);
bool isCopyAssignmentOperatorCall(const CallEvent& Call);
bool isMoveAssignemntCall(const CallEvent &Call);
bool isMoveConstructorCall(const CallEvent &Call);
CallEventRef<> getCaller(const CallEvent &Call, CheckerContext &C);
bool isStdType(const Type *Type, const std::string &TypeName);
bool isStdVariant(const Type *Type);
bool isStdAny(const Type *Type);

template <class T>
void bindFromVariant(const BinaryOperator *BinOp, CheckerContext &C, const CallDescription &StdGet) {
  // consider removing these prints
  llvm::errs() << "Bin op ok\n";
  if (!BinOp->isAssignmentOp()) {
    return;
  }

  auto RHSExpr = BinOp->getRHS();

  if (!RHSExpr) {
    return;
  }

  auto RHSCall = dyn_cast<CallExpr>(RHSExpr);
  auto RHSCast = dyn_cast<CastExpr>(RHSExpr);
  while (!RHSCall && RHSCast) {
    auto SubExpr = RHSCast->getSubExpr();
    if (!SubExpr) {
      return;
    }
    RHSCall = dyn_cast<CallExpr>(SubExpr);
    RHSCast = dyn_cast<CastExpr>(SubExpr);
  }

  if (!RHSCall) {
    return;
  }

  if (!StdGet.matchesAsWritten(*RHSCall)) {
    return;
  }
  llvm::errs() << "matches\n";
    
  if (RHSCall->getNumArgs() != 1) {
    return;
  }
  // We know that at this point we assign value to the LValue on the left from
  // and std::variant

  // what if we assign value to std::variant or std::any the map should still
  // be updated  
  auto Arg = RHSCall->getArg(0);
  if (!Arg) {
    llvm::errs() << "Can not get arg\n";
    return;
  }
  Arg->dump();
  auto ArgDeclRef = dyn_cast<DeclRefExpr>(Arg);
  auto VDecl = dyn_cast<VarDecl>(ArgDeclRef->getDecl());
  llvm::errs() << "\nVDecl\n";
  VDecl->dump();
  llvm::errs() << "\nVDecl\n";


  auto ArgSVal = C.getStoreManager().getLValueVar(VDecl, C.getLocationContext());//C.getSVal(Arg);
  llvm::errs() << "\nArg Sval type\n" << ArgSVal.getType(C.getASTContext()).getAsString() << '\n';
  ArgSVal.dump();
  auto ArgMemRegion = ArgSVal.getAsRegion();
  if (ArgMemRegion) {
    llvm::errs() << "\nmem reg found\n";
  } else {
    llvm::errs() << "\n not mem\n";
    return;
  }
  llvm::errs() << "\nSVal\n";
  auto State = C.getState();
  llvm::errs() << "\n";
  State->dump();
  llvm::errs() << "\n";
  //add check if
  auto SValGet = State->get<T>(ArgMemRegion);
  if (SValGet) {
    llvm::errs() << "\nGood news\n";
    SValGet->dump();
    llvm::errs() << "\naaa\n";
  }

  auto LeftHandExpr = BinOp->getLHS();
  llvm::errs() << "\nlhs\n";
  LeftHandExpr->dump();
  llvm::errs() << '\n';
  auto LHSSVal = C.getSVal(LeftHandExpr);
  llvm::errs() << "\nLeft hand\n";
  LHSSVal.dump();
  llvm::errs() << "\n";
  auto LHSLoc = dyn_cast<Loc>(LHSSVal);
  if (!LHSLoc) {
    llvm::errs() << "\nPls\n";
    return;
  }
  State = State->killBinding(*LHSLoc);
  llvm::errs() << "\nState no bindState\n";
  State->dump();
  llvm::errs() << '\n';


  State = State->bindLoc(*LHSLoc, *SValGet, C.getLocationContext());
  llvm::errs() << "\nNew State\n";
  State->dump();
  llvm::errs() << '\n';

  C.addTransition(State);
}

template <class T, class U>
void handleConstructorAndAssignment(const CallEvent &Call,
                                      CheckerContext &C,
                                      const SVal &thisSVal) {
    auto State = Call.getState(); // check
    auto argQType = Call.getArgSVal(0).getType(C.getASTContext());
    const Type* ArgTypePtr = argQType.getTypePtr();
    auto ThisRegion = thisSVal.getAsRegion();
    auto ArgSVal = Call.getArgSVal(0);
    llvm::errs() << "\n";
    ArgSVal.dump();
    llvm::errs() << "\n";
    auto AsMemRegSVal = dyn_cast<Loc>(ArgSVal);
    if (!AsMemRegSVal) {
      llvm::errs() << "\nNot Loc\n";
    } else {
      llvm::errs() << "\nGood\n";
      ArgSVal = C.getStoreManager().getBinding(C.getState()->getStore(), *AsMemRegSVal);
    }

    auto ArgMemRegion = Call.getArgSVal(0).getAsRegion();

    State = [&]() {if (isCopyConstructorCallEvent(Call) ||
                                          isCopyAssignmentOperatorCall(Call)) {
        // if the argument of a copy constructor or assignment is unknown then
        // we will not know the argument of the copied to object
        if (!State->contains<T>(ArgMemRegion)) {// Think of the case when other is unknown
          return State->remove<T>(ThisRegion);
        } 
        auto OtherQType = State->get<T>(ArgMemRegion);
        auto OtherSVal = State->get<U>(ArgMemRegion);
        State = State->set<U>(ThisRegion, *OtherSVal);
        return State->set<T>(ThisRegion, *OtherQType);
      } else if(isMoveConstructorCall(Call) || isMoveAssignemntCall(Call)) {
        if (!State->contains<T>(ArgMemRegion)) {// Think of the case when other is unknown
          return State->remove<T>(ThisRegion);
        }
        auto OtherQType = State->get<T>(ArgMemRegion);
        State = State->remove<T>(ArgMemRegion);
        return State->set<T>(ThisRegion, *OtherQType);
      } else {
        auto WoPointer = ArgTypePtr->getPointeeType();
        State = State->set<U>(ThisRegion, ArgSVal);
        return State->set<T>(ThisRegion, WoPointer);
    }}();

    if (State) {
      C.addTransition(State);
    } else {
      C.addTransition(Call.getState()->remove<T>(ThisRegion));
    }
}

} //namespace variant_modeling
} //namespace ento
} //namespace clang

#endif // LLVM_CLANG_LIB_STATICANALYZER_CHECKER_VARIANTLIKETYPEMODELING_H
