//===- StdAnyChecker.cpp -------------------------------------*- C++ -*----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//  TODO
//===----------------------------------------------------------------------===//

#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallDescription.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "llvm/ADT/FoldingSet.h"

#include <string>
#include "VariantLikeTypeModeling.h"

using namespace clang;
using namespace ento;
using namespace variant_modeling;

REGISTER_MAP_WITH_PROGRAMSTATE(AnyHeldMap, const MemRegion*, QualType)
REGISTER_MAP_WITH_PROGRAMSTATE(AnyMap, const MemRegion*, SVal)


class StdAnyChecker : public Checker<check::PreCall,
                                     check::RegionChanges,
                                     check::PostStmt<BinaryOperator>> {
  CallDescription AnyConstructorCall{{"std", "any"}};
  CallDescription AnyAsOp{{"std", "any", "operator="}};
  CallDescription AnyReset{{"std", "any", "reset"}};
  CallDescription AnyCast{{"std", "any_cast"}};
  BugType BadAnyType{this, "BadAnyType", "BadAnyType"};

  BugType NullAnyType{this, "NullAnyType", "NullAnyType"};
  
  public:
  void printState(raw_ostream &Out, ProgramStateRef State,
                          const char *NL, const char *Sep) const override {
    // consider using this
  }
  void checkPostStmt(const BinaryOperator *BinOp, CheckerContext &C) const {
    bindFromVariant<AnyMap>(BinOp, C, AnyCast);
  }

  ProgramStateRef checkRegionChanges(ProgramStateRef State,
                                    const InvalidatedSymbols *Invalidated,
                                    ArrayRef<const MemRegion *> ExplicitRegions,
                                    ArrayRef<const MemRegion *> Regions,
                                    const LocationContext *LCtx,
                                    const CallEvent *Call) const {
    if (!Call) {
      return State;
    }

    if (Call->isInSystemHeader()) {
      return State;
    }

    for (auto currentMemRegion : Regions) {
      if (State->contains<AnyHeldMap>(currentMemRegion)) {
        State = State->remove<AnyHeldMap>(currentMemRegion);
      }
    }
    return State;
  }

  void checkPreCall(const CallEvent& Call, CheckerContext& C) const {
    auto Caller = getCaller(Call, C);
    if (Caller) {
      if (Caller->isInSystemHeader()) {
        return;
      }
    }
    
    if (AnyCast.matches(Call)) {
      handleAnyCastCall(Call, C);
      return;
    }
    
    if (AnyReset.matches(Call)) {
      auto AsMemberCall = dyn_cast<CXXMemberCall>(&Call);
      if (!AsMemberCall) {
        return;
      }
      auto ThisMemRegion = AsMemberCall->getCXXThisVal().getAsRegion();
      if(!ThisMemRegion) {
        return;
      }
      setNullTypeAny(ThisMemRegion, C);
      return;
    }

    bool IsAnyConstructor = isa<CXXConstructorCall>(Call) &&
                                          AnyConstructorCall.matches(Call);
    bool IsAnyAssignmentOperatorCall = isa<CXXMemberOperatorCall>(Call) &&
                                                      AnyAsOp.matches(Call);

    if (IsAnyConstructor || IsAnyAssignmentOperatorCall) {
      auto State = C.getState();
      SVal ThisSVal = [&]() {
        if (IsAnyConstructor) {
          auto AsConstructorCall = dyn_cast<CXXConstructorCall>(&Call);
          return AsConstructorCall->getCXXThisVal();
        } else if (IsAnyAssignmentOperatorCall) {
          auto AsMemberOpCall = dyn_cast<CXXMemberOperatorCall>(&Call);
          return AsMemberOpCall->getCXXThisVal();
        } else {
          llvm_unreachable(
                          "We must have an assignment operator or constructor");
        }
      }();

      // default constructor call
      // in this case the any holds a null type
      if(Call.getNumArgs() == 0) {
        auto ThisMemRegion = ThisSVal.getAsRegion();
        setNullTypeAny(ThisMemRegion, C);
        return;
      }

      if (Call.getNumArgs() != 1) {
        return;
      }

      handleConstructorAndAssignment<AnyHeldMap, AnyMap>(Call, C, ThisSVal);
      return;
    }
  }

  private:
  void setNullTypeAny(const MemRegion *Mem, CheckerContext &C) const {
    auto State = C.getState();
    State = State->set<AnyHeldMap>(Mem, QualType{});
    C.addTransition(State);
  }

  //this function name is terrible
  void handleAnyCastCall(const CallEvent &Call, CheckerContext &C) const {
    auto State = C.getState();

    if (Call.getNumArgs() != 1) {
      return;
    }
    auto ArgSVal = Call.getArgSVal(0);
    //??
    auto ArgType = ArgSVal.getType(C.getASTContext()).getTypePtr()->getPointeeType().getTypePtr();
    if (!isStdAny(ArgType)) {
      return;
    }

    // get the type we are trying to get from any
    auto FirstTemplateArgument = getFirstTemplateArgument(Call);
    if (FirstTemplateArgument.getKind() != TemplateArgument::ArgKind::Type) {
      return;
    }

    auto TypeOut = FirstTemplateArgument.getAsType();
    auto AnyMemRegion = ArgSVal.getAsRegion();

    if (!State->contains<AnyHeldMap>(AnyMemRegion)) {
      return;
    }
    auto TypeStored = State->get<AnyHeldMap>(AnyMemRegion);
    if(TypeStored->isNull()) {
      ExplodedNode* ErrNode = C.generateNonFatalErrorNode();
      if (!ErrNode)
        return;
      llvm::SmallString<128> Str;
      llvm::raw_svector_ostream OS(Str);
      OS << "any " << AnyMemRegion->getDescriptiveName() << " held a null type";
      auto R = std::make_unique<PathSensitiveBugReport>(
        NullAnyType, OS.str(), ErrNode);
      C.emitReport(std::move(R));  
      return;
    }

    if (*TypeStored == TypeOut || isObjectOf(TypeOut, *TypeStored)) {
      return;
    }

    ExplodedNode* ErrNode = C.generateNonFatalErrorNode();
    if (!ErrNode)
      return;
    llvm::SmallString<128> Str;
    llvm::raw_svector_ostream OS(Str);
    OS << "std::any "
       << AnyMemRegion->getDescriptiveName()
       << " held a(n) "
       << TypeStored->getAsString()
       << " not a(n) "
       << TypeOut.getAsString();
    auto R = std::make_unique<PathSensitiveBugReport>(
      BadAnyType, OS.str(), ErrNode);
    C.emitReport(std::move(R));  
    return;
  }
};

bool clang::ento::shouldRegisterStdAnyChecker(
    clang::ento::CheckerManager const &mgr) {
  return true;
}

void clang::ento::registerStdAnyChecker(clang::ento::CheckerManager &mgr) {
  mgr.registerChecker<StdAnyChecker>();
}
