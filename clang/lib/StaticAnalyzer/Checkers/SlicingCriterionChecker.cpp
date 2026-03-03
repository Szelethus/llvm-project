#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallDescription.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/SVals.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/VirtualFileSystem.h"
#include <algorithm>
#include <iterator>
#include <optional>
#include <string>
// Run with: ninja clang && \
// bin/clang++ --analyze -Xclang -analyzer-checker=core,alpha.core.SlicingCriterion \
// -Xclang -analyzer-output=html \
// -Xclang -analyzer-config -Xclang alpha.core.SlicingCriterion:LineNumber=12 \
// -Xclang -analyzer-config -Xclang alpha.core.SlicingCriterion:ExpressionName=aaa \
// test.c -o outp
using namespace clang;
using namespace ento;

struct FileIDLine {
  FileID FID;
  unsigned LineNo;
  std::string FnName;

  FileIDLine(FileID FID, unsigned LineNo, std::string FnName)
      : FID(FID), LineNo(LineNo), FnName(FnName) {}
};

static bool operator<(const FileIDLine &lhs, const FileIDLine &rhs) {
  if (lhs.FID == rhs.FID)
    return lhs.LineNo < rhs.LineNo;
  return lhs.FID < rhs.FID;
}

namespace slicing {
class NamedExprVisitor : public RecursiveASTVisitor<NamedExprVisitor> {
public:
  std::vector<const Expr *> NamedExprs;

  bool VisitDeclRefExpr(DeclRefExpr *DRE) {
    NamedExprs.push_back(DRE);
    return true;
  }

  bool VisitMemberExpr(MemberExpr *ME) {
    NamedExprs.push_back(ME);
    return true;
  }

  bool VisitUnresolvedLookupExpr(UnresolvedLookupExpr *ULE) {
    NamedExprs.push_back(ULE);
    return true;
  }

  void traverse(const Stmt *S) { TraverseStmt(const_cast<Stmt *>(S)); }
};
}; // end of namespace slicing

struct SlicingCriterionOptions {
  int LineNumber;
  std::string ExpressionName;
};

static std::vector<const Expr *> findNamedExprsInStmt(const Stmt *S) {
  

  slicing::NamedExprVisitor V{};
  V.traverse(S);

  // for (const Expr *E : V.NamedExprs) {
  //   E->dump(); // or print name with:
  //   if (const auto *DRE = dyn_cast<DeclRefExpr>(E)) {
  //     llvm::errs() << "  Name: " << DRE->getNameInfo().getAsString() << '\n';
  //   } else if (const auto *ME = dyn_cast<MemberExpr>(E)) {
  //     llvm::errs() << "  Member: " << ME->getMemberDecl()->getNameAsString()
  //                  << '\n';
  //   } else if (const auto *ULE = dyn_cast<UnresolvedLookupExpr>(E)) {
  //     llvm::errs() << "  Unresolved name: " << ULE->getName().getAsString()
  //                  << '\n';
  //   }
  // }
  return V.NamedExprs;
}

static std::optional<std::string> getNameForNamedExpression(const Expr *Ex) {
  if (const auto *DRE = dyn_cast<DeclRefExpr>(Ex)) {
    return DRE->getNameInfo().getAsString();
  }
  if (const auto *ME = dyn_cast<MemberExpr>(Ex)) {
    llvm::errs() << "MEMBER EXPR\n";
    llvm::errs() << ME->getMemberNameInfo().getName() << '\n';
    return ME->getMemberDecl()->getNameAsString();
  }
  if (const auto *ULE = dyn_cast<UnresolvedLookupExpr>(Ex)) {
    return ULE->getName().getAsString();
  }
  return {};
}

static std::optional<const Expr *> namedExpressionPresentInStmt(const Stmt *S,
                                         const std::string& Name) {
  auto GetNameForNamedExpression = [](const Expr *Ex) -> std::string {
    if (const auto *DRE = dyn_cast<DeclRefExpr>(Ex)) {
      return DRE->getNameInfo().getAsString();
    }
    if (const auto *ME = dyn_cast<MemberExpr>(Ex)) {
      return ME->getMemberDecl()->getNameAsString();
    }
    if (const auto *ULE = dyn_cast<UnresolvedLookupExpr>(Ex)) {
      return ULE->getName().getAsString();
    }
    return std::string("");
  };

  std::vector<const Expr *> ExpressionsInStmt = findNamedExprsInStmt(S);
  if (!ExpressionsInStmt.size())
    return {};

  auto ExWithTheNameIt = std::find_if(ExpressionsInStmt.begin(),
               ExpressionsInStmt.end(),
               [Name, &GetNameForNamedExpression](const Expr *Ex) {
                 return GetNameForNamedExpression(Ex) == Name;
               });

  if (ExWithTheNameIt == ExpressionsInStmt.end())
    return {};

  return *ExWithTheNameIt;
}

class SlicingCriterionReport : public PathSensitiveBugReport {
  int x = 0;
public:
  SlicingCriterionReport(const BugType &bt, StringRef desc,
                         const ExplodedNode *errorNode)
      : PathSensitiveBugReport(bt, desc, errorNode) {}

  void Profile(llvm::FoldingSetNodeID &hash) const override {
    this->PathSensitiveBugReport::Profile(hash);
    hash.AddPointer(&x);
  }
  
};

static void printFileAndLine(llvm::raw_ostream &out, const SourceManager &SM,
                             FileID FID, int line) {
  assert(FID.isValid());
  assert(SM.getFileEntryRefForID(FID).has_value());
  llvm::StringRef fullPath =
      SM.getFileEntryRefForID(FID)->getName();
  out << fullPath << ' ' << line;
}

class SlicingCriterionChecker : public Checker<check::PreStmt<Stmt>> {
  mutable std::set<FileIDLine> s;
public:
  SlicingCriterionOptions Opts;
  BugType SlicingCriterionFound{this, "SlicingCriterionFound", "SlicingCriterionFound"};

  void checkPreStmt(const Stmt *S, CheckerContext &C) const {
    const SourceManager &SM = C.getSourceManager();
    clang::FileID FID = SM.getFileID(SM.getSpellingLoc(S->getBeginLoc()));
    unsigned line = SM.getSpellingLineNumber(S->getBeginLoc());
 
    // Assuming 'SFC' is a const clang::StackFrameContext *
    const clang::Decl *D = C.getStackFrame()->getDecl();
    
    // Automatically handles functions, methods, and blocks
    std::string FuncName = clang::AnalysisDeclContext::getFunctionName(D);

    s.insert({FID, line, FuncName});

    if (line != (unsigned)Opts.LineNumber)
      return;

    // We know that we are in the correct line.
    std::optional<const Expr *> Ex = namedExpressionPresentInStmt(S, Opts.ExpressionName);
    if (!Ex) {
      llvm::errs() << "The line is fine but no expression named " 
                   << Opts.ExpressionName << '\n'
                   << "The available expression names are the following:\n";
      for (const auto *ex : findNamedExprsInStmt(S)) {
        getNameForNamedExpression(ex);
        llvm::errs() << '\n';
      }
      llvm::errs() << "--\n";
      return;
    }

    llvm::errs() << "SLICING CRITERION FOUND\n";
    ExplodedNode *ErrNode = C.generateNonFatalErrorNode(C.getState());
    if (!ErrNode)
      return;
    llvm::SmallString<128> Str;
    llvm::raw_svector_ostream OS(Str);
    OS << "Slicing Criterion Found";
    //llvm::errs() << "Line: " << SM.getSpellingLineNumber(S->getBeginLoc())
    //           << '\n';
    //llvm::errs() << "SC: " << Opts.LineNumber << ":" << Opts.ExpressionName
    //             << '\n';
    //llvm::errs() << "Entering Slicing Criterion checker for stmt: "
    //             << S->getStmtClassName() << "\n";
    //std::vector<const Expr *> ExpressionsInStmt = findNamedExprsInStmt(S);
    auto R = std::make_unique<SlicingCriterionReport>(SlicingCriterionFound, OS.str(),
                                                      ErrNode);
    bugreporter::trackExpressionValue(ErrNode, *Ex, *R);
    for (auto data : s) {
      if (data.FID.isInvalid())
        continue;
      if (!SM.getFileEntryRefForID(data.FID).has_value())
        continue;
      llvm::errs() << "Executed: ";
      printFileAndLine(llvm::errs(), C.getSourceManager(), data.FID,
                       data.LineNo);
      llvm::errs() << ' ' << data.FnName << '\n';
    }
    C.emitReport(std::move(R));
  }
};

static StringRef rsplit2(StringRef str, StringRef sep1, StringRef sep2) {
  StringRef tmp;
  auto splits = str.rsplit(sep1);
  if (splits.second.empty())
    tmp = splits.first;
  else
    tmp = splits.second;
  splits = tmp.rsplit(sep2);
  if (splits.second.empty())
    tmp = splits.first;
  else
    tmp = splits.second;
  return tmp;
}

void clang::ento::registerSlicingCriterionChecker(
    clang::ento::CheckerManager &Mgr) {
  auto *Chk = Mgr.registerChecker<SlicingCriterionChecker>();

  const AnalyzerOptions &AnOpts = Mgr.getAnalyzerOptions();
  SlicingCriterionOptions &ChOpts = Chk->Opts;
  ChOpts.LineNumber = AnOpts.getCheckerIntegerOption(Chk, "LineNumber");
  ChOpts.ExpressionName =
      rsplit2(AnOpts.getCheckerStringOption(Chk, "ExpressionName"), ".", "->");

  llvm::errs() << "note: Actual expression checked: " << ChOpts.ExpressionName
               << '\n';
}

bool clang::ento::shouldRegisterSlicingCriterionChecker(
    clang::ento::CheckerManager const &mgr) {
  return true;
}
