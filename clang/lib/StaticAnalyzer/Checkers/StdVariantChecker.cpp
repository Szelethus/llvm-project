//===- StdVariantChecker.cpp -------------------------------------*- C++ -*-==//
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

REGISTER_MAP_WITH_PROGRAMSTATE(VariantHeldMap, const MemRegion*, QualType)
REGISTER_MAP_WITH_PROGRAMSTATE(VariantMap, const MemRegion*, SVal)


namespace clang {
namespace ento {
namespace variant_modeling {

CallEventRef<> getCaller(const CallEvent &Call, CheckerContext &C) {
  // should be const auto *, and everywhere else
  auto CallLocationContext = Call.getLocationContext();
  if (!CallLocationContext) {
    return nullptr; 
  }

  if (CallLocationContext->inTopFrame()) {
    // Consider using this trick elsewhere.
    return nullptr; 
  }
  auto CallStackFrameContext = CallLocationContext->getStackFrame();
  if (!CallStackFrameContext) {
    return CallEventRef<>(nullptr);
  }

  CallEventManager &CEMgr = C.getState()->getStateManager().getCallEventManager();
  return CEMgr.getCaller(CallStackFrameContext, C.getState());
}

// MORE COMMENTS, GRAY IS GOOD
bool isObjectOf(QualType t, QualType to) {
  QualType canonicalTypeT = t.getCanonicalType();
  QualType canonicalTypeTo = to.getCanonicalType();
  return canonicalTypeTo == canonicalTypeT && canonicalTypeTo->isObjectType();
}

bool isCopyConstructorCallEvent (const CallEvent& Call) {
  auto ConstructorCall = dyn_cast<CXXConstructorCall>(&Call);
  if (!ConstructorCall) {
    return false;
  }
  auto ConstructorDecl = ConstructorCall->getDecl();
  if (!ConstructorDecl) {
    return false;
  }
  return ConstructorDecl->isCopyConstructor();
}

bool isCopyAssignmentOperatorCall(const CallEvent& Call) {
  auto CopyAssignmentCall = dyn_cast<CXXMemberOperatorCall>(&Call);
  if (!CopyAssignmentCall) {
    return false;
  }
  auto CopyAssignmentDecl = CopyAssignmentCall->getDecl();
  if (!CopyAssignmentDecl) {
    return false;
  }
  auto AsMethodDecl = dyn_cast<CXXMethodDecl>(CopyAssignmentDecl);
  if (!AsMethodDecl) {
    return false;
  }
  return AsMethodDecl->isCopyAssignmentOperator();
}

bool isMoveConstructorCall(const CallEvent &Call) {
  auto ConstructorCall = dyn_cast<CXXConstructorCall>(&Call);
  if (!ConstructorCall) {
    return false;
  }
  auto ConstructorDecl = ConstructorCall->getDecl();
  if (!ConstructorDecl) {
    return false;
  }
  return ConstructorDecl->isMoveConstructor();
}

bool isMoveAssignemntCall(const CallEvent &Call) {
  auto CopyAssignmentCall = dyn_cast<CXXMemberOperatorCall>(&Call);
  if (!CopyAssignmentCall) {
    return false;
  }
  auto CopyAssignmentDecl = CopyAssignmentCall->getDecl();
  if (!CopyAssignmentDecl) {
    return false;
  }
  auto AsMethodDecl = dyn_cast<CXXMethodDecl>(CopyAssignmentDecl);
  if (!AsMethodDecl) {
    return false;
  }
  return AsMethodDecl->isMoveAssignmentOperator();
}

const TemplateArgument& getFirstTemplateArgument(const CallEvent &Call) {
  const CallExpr* CE = cast<CallExpr>(Call.getOriginExpr());
  const FunctionDecl* FD = CE->getDirectCallee();
  assert(1 <= FD->getTemplateSpecializationArgs()->asArray().size() &&
              "std::get should have at least 1 template argument!");
  return FD->getTemplateSpecializationArgs()->asArray()[0];
}

bool isStdType(const Type *Type, const std::string &TypeName) {
  auto Decl = Type->getAsRecordDecl();
  if (!Decl) {
    return false;
  }

  return (Decl->getNameAsString() == TypeName)
          && Decl->isInStdNamespace();
}

bool isStdVariant(const Type *Type) {
  return isStdType(Type, std::string("variant"));
}

bool isStdAny(const Type *Type) {
  return isStdType(Type, std::string("any"));
}

} // end of namespace variant_modeling
} // end of namespace ento
} // end of namespace clang

// Get the non pointer type behind any pointer type
// For example if we have an int*** we get int
static const Type* getPointeeType (const QualType& qt) {
  QualType Type = qt;
  QualType NextType = qt.getTypePtr()->getPointeeType();
  while (!NextType.isNull()) {
    Type = NextType;
    NextType = Type.getTypePtr()->getPointeeType();
  }
  return Type.getTypePtr();
}

static ArrayRef<TemplateArgument> getTemplateArgsFromVariant
                                                    (const Type* VariantType) {
  auto TempSpecType = VariantType->getAs<TemplateSpecializationType>();
  assert(TempSpecType
      && "We are in a variant instance. It must be a template specialization!");
  return TempSpecType->template_arguments();
}

static QualType getNthTmplateTypeArgFromVariant
                                            (const Type* varType, unsigned i) {
  return getTemplateArgsFromVariant(varType)[i].getAsType();
}

// use anonymous namespace
class StdVariantChecker : public Checker<check::PreCall,
                                         check::RegionChanges,
                                         check::PostStmt<BinaryOperator>> {
  CallDescription VariantConstructorCall{{"std", "variant"}};
  CallDescription VariantAsOp{{"std", "variant", "operator="}};
  CallDescription StdGet{{"std", "get"}};
  BugType VariantCreated{this, "VariantCreated", "VariantCreated"};

  public:
  void checkPostStmt(const BinaryOperator *BinOp, CheckerContext &C) const {
    bindFromVariant<VariantMap>(BinOp, C, StdGet);
  }

  // consider moving big dunctions out-of-line
  ProgramStateRef
    checkRegionChanges(ProgramStateRef State,
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
      if (State->contains<VariantHeldMap>(currentMemRegion)) {
        State = State->remove<VariantHeldMap>(currentMemRegion);
      }
    }
    return State;
  }

  void checkPreCall(const CallEvent &Call, CheckerContext &C) const {
    auto Caller = getCaller(Call, C);
    if (Caller) {
      if (Caller->isInSystemHeader()) {
        return;
      }
    }

    if (StdGet.matches(Call)) {
      handleStdGetCall(Call, C);
      return;
    }

    bool IsVariantConstructor = isa<CXXConstructorCall>(Call) &&
                                          VariantConstructorCall.matches(Call);
    bool IsVariantAssignmentOperatorCall = isa<CXXMemberOperatorCall>(Call) &&
                                                      VariantAsOp.matches(Call);

    if (IsVariantConstructor || IsVariantAssignmentOperatorCall) {
      // default ctor
      if (IsVariantConstructor && Call.getNumArgs() == 0) {
        handleDefaultConstructor(Call, C);
        return;
      }
      // explain this
      if (Call.getNumArgs() != 1)
        return;
      // non-default ctor
      SVal thisSVal = [&]() {
        if (IsVariantConstructor) {
          auto AsConstructorCall = dyn_cast<CXXConstructorCall>(&Call);
          return AsConstructorCall->getCXXThisVal();
        } else if (IsVariantAssignmentOperatorCall) {
          auto AsMemberOpCall = dyn_cast<CXXMemberOperatorCall>(&Call);
          return AsMemberOpCall->getCXXThisVal();
        } else {
          llvm_unreachable(
                          "We must have an assignment operator or constructor");
        }
      }();
      handleConstructorAndAssignment<VariantHeldMap, VariantMap>(Call, C, thisSVal);
      return;
    }
  }

private:
  void handleDefaultConstructor(const CallEvent &Call,
                                CheckerContext &C) const {
    auto State = Call.getState();
    auto AsConstructorCall = dyn_cast<CXXConstructorCall>(&Call);
    if (!AsConstructorCall) {
      return;
    }

    auto ThisSVal = AsConstructorCall->getCXXThisVal();
    auto MemRegion = ThisSVal.getAsRegion();
    if (!MemRegion) {
      return;
    }
    auto VariantType = getNthTmplateTypeArgFromVariant(
        getPointeeType(ThisSVal.getType(C.getASTContext())), 0);

    State = State->set<VariantHeldMap>(MemRegion, VariantType);
    C.addTransition(State);
  }

  void handleStdGetCall(const CallEvent &Call, CheckerContext &C) const {
    // neither of these is the time of place for auto
    auto State = Call.getState();
    auto TypeOut = getFirstTemplateArgument(Call);
    auto ArgType = Call.getArgSVal(0).getType(C.getASTContext()).getTypePtr()->
                                      getPointeeType().getTypePtr();
    if (!isStdVariant(ArgType)) {
      return;
    }

    auto ArgMemRegion = Call.getArgSVal(0).getAsRegion();
    auto TypeStored = State->get<VariantHeldMap>(ArgMemRegion);
    if (!TypeStored) {
      return;
    }

    auto GetType = [&]() {
    switch (TypeOut.getKind()) {
      case TemplateArgument::ArgKind::Type:
        return TypeOut.getAsType();
      case TemplateArgument::ArgKind::Integral:
        return getNthTmplateTypeArgFromVariant(
          ArgType,
          TypeOut.getAsIntegral().getSExtValue());
      default:
        llvm_unreachable(
    "An std::get's first template argument can only be a type or an integral");
    }}();

    if (GetType == *TypeStored || isObjectOf(GetType, *TypeStored)) {
      return;
    }

    ExplodedNode* ErrNode = C.generateNonFatalErrorNode();
    if (!ErrNode)
      return;
    llvm::SmallString<128> Str;
    llvm::raw_svector_ostream OS(Str);
    OS << "variant " << ArgMemRegion->getDescriptiveName() << " held a(n) "
       << TypeStored->getAsString()
       << " not a(n) " << GetType.getAsString();
    auto R = std::make_unique<PathSensitiveBugReport>(
      VariantCreated, OS.str(), ErrNode);
    C.emitReport(std::move(R));
  }
};

bool clang::ento::shouldRegisterStdVariantChecker(
    clang::ento::CheckerManager const &mgr) {
  return true;
}

void clang::ento::registerStdVariantChecker(clang::ento::CheckerManager &mgr) {
  mgr.registerChecker<StdVariantChecker>();
}
