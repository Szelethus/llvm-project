//===--- ASTConsumers.cpp - ASTConsumer implementations -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AST Consumer Implementations.
//
//===----------------------------------------------------------------------===//

#include "clang/Frontend/ASTConsumers.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTFwd.h"
#include "clang/AST/Decl.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtCXX.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;

//===----------------------------------------------------------------------===//
/// ASTPrinter - Pretty-printer and dumper of ASTs

namespace {
class ASTPrinter : public ASTConsumer, public RecursiveASTVisitor<ASTPrinter> {
  typedef RecursiveASTVisitor<ASTPrinter> base;

public:
  enum Kind { DumpFull, Dump, Print, None };
  ASTPrinter(std::unique_ptr<raw_ostream> Out, Kind K,
             ASTDumpOutputFormat Format, StringRef FilterString,
             bool DumpLookups = false, bool DumpDeclTypes = false)
      : Out(Out ? *Out : llvm::outs()), OwnedOut(std::move(Out)), OutputKind(K),
        OutputFormat(Format), FilterString(FilterString),
        DumpLookups(DumpLookups), DumpDeclTypes(DumpDeclTypes) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    TranslationUnitDecl *D = Context.getTranslationUnitDecl();

    if (FilterString.empty())
      return print(D);

    TraverseDecl(D);
  }

  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool TraverseDecl(Decl *D) {
    if (D && filterMatches(D)) {
      bool ShowColors = Out.has_colors();
      if (ShowColors)
        Out.changeColor(raw_ostream::BLUE);
      Out << (OutputKind != Print ? "Dumping " : "Printing ") << getName(D)
          << ":\n";
      if (ShowColors)
        Out.resetColor();
      print(D);
      Out << "\n";
      // Don't traverse child nodes to avoid output duplication.
      return true;
    }
    return base::TraverseDecl(D);
  }

private:
  std::string getName(Decl *D) {
    if (isa<NamedDecl>(D))
      return cast<NamedDecl>(D)->getQualifiedNameAsString();
    return "";
  }
  bool filterMatches(Decl *D) {
    return getName(D).find(FilterString) != std::string::npos;
  }
  void print(Decl *D) {
    if (DumpLookups) {
      if (DeclContext *DC = dyn_cast<DeclContext>(D)) {
        if (DC == DC->getPrimaryContext())
          DC->dumpLookups(Out, OutputKind != None, OutputKind == DumpFull);
        else
          Out << "Lookup map is in primary DeclContext "
              << DC->getPrimaryContext() << "\n";
      } else
        Out << "Not a DeclContext\n";
    } else if (OutputKind == Print) {
      PrintingPolicy Policy(D->getASTContext().getLangOpts());
      D->print(Out, Policy, /*Indentation=*/0, /*PrintInstantiation=*/true);
    } else if (OutputKind != None) {
      D->dump(Out, OutputKind == DumpFull, OutputFormat);
    }

    if (DumpDeclTypes) {
      Decl *InnerD = D;
      if (auto *TD = dyn_cast<TemplateDecl>(D))
        InnerD = TD->getTemplatedDecl();

      // FIXME: Support OutputFormat in type dumping.
      // FIXME: Support combining -ast-dump-decl-types with -ast-dump-lookups.
      if (auto *VD = dyn_cast<ValueDecl>(InnerD))
        VD->getType().dump(Out, VD->getASTContext());
      if (auto *TD = dyn_cast<TypeDecl>(InnerD))
        TD->getTypeForDecl()->dump(Out, TD->getASTContext());
    }
  }

  raw_ostream &Out;
  std::unique_ptr<raw_ostream> OwnedOut;

  /// How to output individual declarations.
  Kind OutputKind;

  /// What format should the output take?
  ASTDumpOutputFormat OutputFormat;

  /// Which declarations or DeclContexts to display.
  std::string FilterString;

  /// Whether the primary output is lookup results or declarations. Individual
  /// results will be output with a format determined by OutputKind. This is
  /// incompatible with OutputKind == Print.
  bool DumpLookups;

  /// Whether to dump the type for each declaration dumped.
  bool DumpDeclTypes;
};

class ASTDeclNodeLister : public ASTConsumer,
                          public RecursiveASTVisitor<ASTDeclNodeLister> {
public:
  ASTDeclNodeLister(raw_ostream *Out = nullptr)
      : Out(Out ? *Out : llvm::outs()) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    TraverseDecl(Context.getTranslationUnitDecl());
  }

  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool VisitNamedDecl(NamedDecl *D) {
    D->printQualifiedName(Out);
    Out << '\n';
    return true;
  }

private:
  raw_ostream &Out;
};
} // end anonymous namespace

std::unique_ptr<ASTConsumer>
clang::CreateASTPrinter(std::unique_ptr<raw_ostream> Out,
                        StringRef FilterString) {
  return std::make_unique<ASTPrinter>(std::move(Out), ASTPrinter::Print,
                                      ADOF_Default, FilterString);
}

std::unique_ptr<ASTConsumer>
clang::CreateASTDumper(std::unique_ptr<raw_ostream> Out, StringRef FilterString,
                       bool DumpDecls, bool Deserialize, bool DumpLookups,
                       bool DumpDeclTypes, ASTDumpOutputFormat Format) {
  assert((DumpDecls || Deserialize || DumpLookups) && "nothing to dump");
  return std::make_unique<ASTPrinter>(std::move(Out),
                                      Deserialize ? ASTPrinter::DumpFull
                                      : DumpDecls ? ASTPrinter::Dump
                                                  : ASTPrinter::None,
                                      Format, FilterString, DumpLookups,
                                      DumpDeclTypes);
}

std::unique_ptr<ASTConsumer> clang::CreateASTDeclNodeLister() {
  return std::make_unique<ASTDeclNodeLister>(nullptr);
}

//===----------------------------------------------------------------------===//
// IntVectorDumper
//===----------------------------------------------------------------------===//

namespace {
struct FunctionInfo {
  std::string FileName, FunctionName;
  llvm::SmallVector<int, 32> Infos;
  std::pair<int, int> BeginLoc, EndLoc;
  bool IsSystemHeaderFunction;

  enum class InfoKind {
#define STMT(E, Base) E##Count,
#include "clang/AST/StmtNodes.inc"
#undef STMT

#define ABSTRACT_DECL(TYPE)
#define DECL(E, Base) E##DeclCount,
#include "clang/AST/DeclNodes.inc"
#undef DECL
#undef ABSTRACT_DECL

#define TYPE(E, Base) E##TypeCount,
#include "clang/AST/TypeNodes.inc"
#undef TYPE
    END
  };

  static StringRef infoKindToString(InfoKind k) {
    switch (k) {
#define STMT(E, Base)                                                          \
  case InfoKind::E##Count:                                                     \
    return #E " count";
#include "clang/AST/StmtNodes.inc"
#undef STMT

#define ABSTRACT_DECL(TYPE)
#define DECL(E, Base)                                                          \
  case InfoKind::E##DeclCount:                                                 \
    return #E "Decl count";
#include "clang/AST/DeclNodes.inc"
#undef DECL
#undef ABSTRACT_DECL

#define TYPE(E, Base)                                                          \
  case InfoKind::E##TypeCount:                                                 \
    return #E "Type count";
#include "clang/AST/TypeNodes.inc"
#undef TYPE
    case InfoKind::END:
      llvm_unreachable("");
    }
    llvm_unreachable("Unknown infokind!");
  }

  static std::pair<int, int> getLineColumnIntPair(SourceLocation R,
                                                  const SourceManager &SM) {
    return {SM.getSpellingLineNumber(R), SM.getSpellingColumnNumber(R)};
  }

  FunctionInfo(NamedDecl *ND, const ASTContext *ACtx)
      : FileName(ACtx->getSourceManager().getFilename(ND->getLocation())),
        FunctionName(ND->getDeclName().getAsString()),
        Infos(static_cast<int>(InfoKind::END), 0),
        BeginLoc(getLineColumnIntPair(ND->getSourceRange().getBegin(),
                                      ACtx->getSourceManager())),
        EndLoc(getLineColumnIntPair(ND->getSourceRange().getEnd(),
                                    ACtx->getSourceManager())),
        IsSystemHeaderFunction(
            ACtx->getSourceManager().isInSystemHeader(ND->getLocation())) {}

  template <InfoKind K> int &getCountMutable() {
    assert(static_cast<int>(K) < Infos.size());
    return Infos[static_cast<int>(K)];
  }

  static void dumpColumnNamesToStream(llvm::raw_ostream &out) {
    llvm::SmallString<200> Str;
    llvm::raw_svector_ostream OS(Str);
    OS << "Filename,Function name,System header function,Begin line,Begin column,End line,End column,";
    for (size_t I = 0; I < static_cast<int>(InfoKind::END); ++I)
      OS << infoKindToString(static_cast<InfoKind>(I)) << ',';
    Str.pop_back();
    OS << '\n';
    out << Str;
  }

  void dumpToStream(llvm::raw_ostream &out) const {
    llvm::SmallString<200> Str;
    llvm::raw_svector_ostream OS(Str);
    OS << '\"' << FileName << "\",";
    OS << '\"' << FunctionName << "\",";
    OS << (IsSystemHeaderFunction ? "true" : "false") << ',';
    OS << BeginLoc.first << ',' << BeginLoc.second << ',' << EndLoc.first << ',' << EndLoc.second << ',';
    for (size_t I = 0; I < Infos.size(); ++I)
      OS << Infos[I] << ',';
    Str.pop_back();
    OS << '\n';
    out << Str;
  }

  FunctionInfo() = default;
};
} // namespace

namespace {
class IntVectorDumper : public ASTConsumer {
  ASTContext *Context;
  llvm::SmallVector<FunctionInfo, 20> FunctionInfos;
  std::string File;

public:
  IntVectorDumper(StringRef File) : File(File.str()) {}
  ~IntVectorDumper() {
    if (File == "") {
      dumpToStream(llvm::errs());
      return;
    }
    std::error_code EC;
    llvm::raw_fd_ostream O(File, EC, llvm::sys::fs::OF_TextWithCRLF);
    assert(!O.has_error());
    dumpToStream(O);
  }
  void Initialize(ASTContext &Context) override { this->Context = &Context; }

  bool HandleTopLevelDecl(DeclGroupRef D) override {
    for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I)
      HandleTopLevelSingleDecl(*I);
    return true;
  }

  void dumpToStream(llvm::raw_ostream &out) const;

  void HandleTopLevelSingleDecl(Decl *D);
};
} // namespace

class DataCollectorVisitor : public RecursiveASTVisitor<DataCollectorVisitor> {
  FunctionInfo &Info;

  using InfoKind = FunctionInfo::InfoKind;

public:
  DataCollectorVisitor(FunctionInfo &Info) : Info(Info) {}

#define STMT(E, Base)                                                          \
  bool Visit##E(E *) {                                                         \
    ++Info.getCountMutable<InfoKind::E##Count>();                              \
    return true;                                                               \
  }
#include "clang/AST/StmtNodes.inc"
#undef STMT

#define ABSTRACT_DECL(TYPE)
#define DECL(E, Base)                                                          \
  bool Visit##E##Decl(E##Decl *) {                                             \
    ++Info.getCountMutable<InfoKind::E##DeclCount>();                          \
    return true;                                                               \
  }
#include "clang/AST/DeclNodes.inc"
#undef DECL
#undef ABSTRACT_DECL

#define TYPE(E, Base)                                                          \
  bool Visit##E##Type(E##Type *) {                                             \
    ++Info.getCountMutable<InfoKind::E##TypeCount>();                          \
    return true;                                                               \
  }
#include "clang/AST/TypeNodes.inc"
#undef TYPE
};

struct FunctionVisitor : public RecursiveASTVisitor<FunctionVisitor> {
  llvm::SmallVector<FunctionInfo, 20> &FunctionInfos;
  ASTContext &ACtx;

  FunctionVisitor(llvm::SmallVector<FunctionInfo, 20> &FunctionInfos,
                  ASTContext &ACtx)
      : FunctionInfos(FunctionInfos), ACtx(ACtx) {}

  bool VisitFunctionDecl(FunctionDecl *FD) {
    FunctionInfos.emplace_back(FD, &ACtx);
    DataCollectorVisitor Visitor(FunctionInfos.back());
    Visitor.TraverseDecl(FD);
    return true;
  }
};

void IntVectorDumper::HandleTopLevelSingleDecl(Decl *D) {
  if (Context->getSourceManager().isInSystemHeader(D->getLocation()))
    return;
  FunctionVisitor FV(FunctionInfos, *Context);

  FV.TraverseDecl(D);
}

void IntVectorDumper::dumpToStream(llvm::raw_ostream &out) const {
  FunctionInfo::dumpColumnNamesToStream(out);
  for (const FunctionInfo &FI : FunctionInfos) {
    FI.dumpToStream(out);
    out.flush();
  }
}

std::unique_ptr<ASTConsumer> clang::CreateIntVectorDumper(StringRef File) {
  return std::make_unique<IntVectorDumper>(File);
}

//===----------------------------------------------------------------------===//
/// ASTViewer - AST Visualization

namespace {
class ASTViewer : public ASTConsumer {
  ASTContext *Context;

public:
  void Initialize(ASTContext &Context) override { this->Context = &Context; }

  bool HandleTopLevelDecl(DeclGroupRef D) override {
    for (DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I)
      HandleTopLevelSingleDecl(*I);
    return true;
  }

  void HandleTopLevelSingleDecl(Decl *D);
};
} // namespace

void ASTViewer::HandleTopLevelSingleDecl(Decl *D) {
  if (isa<FunctionDecl>(D) || isa<ObjCMethodDecl>(D)) {
    D->print(llvm::errs());

    if (Stmt *Body = D->getBody()) {
      llvm::errs() << '\n';
      Body->viewAST();
      llvm::errs() << '\n';
    }
  }
}

std::unique_ptr<ASTConsumer> clang::CreateASTViewer() {
  return std::make_unique<ASTViewer>();
}
