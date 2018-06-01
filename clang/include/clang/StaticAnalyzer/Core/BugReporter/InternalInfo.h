//===- InternalInfo.h -------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the InternalInfo interfaces.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CORE_BUGREPORTER_INTERNALINFO_H
#define LLVM_CLANG_STATICANALYZER_CORE_BUGREPORTER_INTERNALINFO_H

#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramState.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"

namespace clang {

namespace markup {

using FIDMap = llvm::DenseMap<FileID, unsigned>;

} // end of namespace markup

namespace ento {

/// Abstract interface for storing debug and various other information.
class InternalInfo : public llvm::FoldingSetNode {
public:
  /// All descendants must add a new kind.
  enum Kind { None, Constraint, State };

private:
  const Kind kind;

protected:
  InternalInfo(Kind kind) : kind(kind) {}

public:
  virtual ~InternalInfo() = default;

  Kind getKind() const { return kind; }

  /// All descendants must override this method and return with their kind.
  static constexpr Kind getStaticKind() { return None; }

  static bool isSameKind(const InternalInfo &Info) {
    return Info.getKind() == getStaticKind();
  }

  /// Profile - Used to profile the contents of this object for inclusion in a
  /// FoldingSet.
  void Profile(llvm::FoldingSetNodeID &ID) const {
    ID.AddInteger(static_cast<int>(kind));
  }

  template <typename T> T *getAs() {
    return T::isSameKind(*this) ? static_cast<T *>(this) : nullptr;
  }

  virtual void addFIDs(markup::FIDMap &FIDs, SmallVectorImpl<FileID> &V,
                       const SourceManager &SM) const = 0;

  virtual void printPlist(llvm::raw_ostream &Out, const SourceManager &SM,
                          const markup::FIDMap &FM, unsigned Indent) const = 0;

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  LLVM_DUMP_METHOD void dump(const SourceManager &SM) const {
    dumpToStream(llvm::errs(), SM);
  }
  LLVM_DUMP_METHOD virtual void dumpToStream(llvm::raw_ostream &Out,
                                             const SourceManager &SM) const = 0;
#endif
};

/// A wrapper around llvm::FoldingSet<InternalInfo>.
class InternalInfoMap {
  using InternalInfoMapImpl = llvm::FoldingSet<InternalInfo>;

public:
  using iterator = InternalInfoMapImpl::iterator;
  using const_iterator = InternalInfoMapImpl::const_iterator;
  using value_type = InternalInfo;

private:
  llvm::FoldingSet<InternalInfo> Set;

public:
  ~InternalInfoMap() {
    for (const auto &E : Set)
      delete &E;
  }

  bool empty() const { return Set.empty(); }
  iterator begin() { return Set.begin(); }
  const_iterator begin() const { return Set.begin(); }
  iterator end() { return Set.end(); }
  const_iterator end() const { return Set.end(); }

  /// Get an InternalInfo object from the map. If the map doesn't contain an
  /// object of type \p T, it inserts it first.
  ///
  /// \param T - Descendant of InternalInfo.
  template <typename T, typename... Args> T &getOrInsert(Args &&... CtorArgs) {
    llvm::FoldingSetNodeID ID;
    ID.AddInteger(static_cast<int>(T::getStaticKind()));
    void *Pos;

    InternalInfo *Val = Set.FindNodeOrInsertPos(ID, Pos);
    if (!Val) {
      Val = new T(std::forward<Args>(CtorArgs)...);
      Set.InsertNode(Val, Pos);
    }

    return static_cast<T &>(*Val);
  }

  /// Get an InternalInfo object from the map. Assert if it's not in the map.
  ///
  /// \param T - Descendant of InternalInfo.
  template <typename T> T &get() {
    llvm::FoldingSetNodeID ID;
    ID.AddInteger(static_cast<int>(T::getStaticKind()));
    void *Pos;

    InternalInfo *Val = Set.FindNodeOrInsertPos(ID, Pos);
    assert(Val && "The map doesn't contain the specified InternalInfo object!");

    return static_cast<T &>(*Val);
  }

  template <typename T> bool isEnabled() {
    llvm::FoldingSetNodeID ID;
    ID.AddInteger(static_cast<int>(T::getStaticKind()));
    void *Pos;

    InternalInfo *Val = Set.FindNodeOrInsertPos(ID, Pos);
    return Val;
  }

  void addFIDs(markup::FIDMap &FIDs, SmallVectorImpl<FileID> &V,
               const SourceManager &SM) const;

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  LLVM_DUMP_METHOD void dump(const SourceManager &SM) const {
    dumpToStream(llvm::errs(), SM);
  }
  LLVM_DUMP_METHOD void dumpToStream(llvm::raw_ostream &Out,
                                     const SourceManager &SM) const;
#endif
};

} // namespace ento
} // namespace clang

#endif // LLVM_CLANG_STATICANALYZER_CORE_BUGREPORTER_INTERNALINFO_H
