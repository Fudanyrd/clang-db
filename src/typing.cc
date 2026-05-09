#include <llvm/Support/raw_ostream.h>

#include "clangdb.h"

namespace clang {
namespace database {

std::string mangleType(const Type *TypePtr,
                       std::map<std::string, std::string> &Typedefs) {
  if (!TypePtr) {
    llvm::errs() << "Error: null type pointer\n";
    abort();
  }

/**
 * FIXME: map "..." to "z"
 */
#define BuiltInTyKindToString(X)                                               \
  X(SChar, "a")      /* signed char */                                         \
  X(Bool, "b")       /* bool or _Bool */                                       \
  X(Char_S, "c")     /* char */                                                \
  X(Double, "d")     /* double */                                              \
  X(LongDouble, "e") /* long double */                                         \
  X(Float, "f")      /* float */                                               \
  X(Float128, "g")   /* __float128 */                                          \
  X(UChar, "h")      /* unsigned char */                                       \
  X(Int, "i")        /* int */                                                 \
  X(UInt, "j")       /* unsigned int */                                        \
  X(Long, "l")       /* long int */                                            \
  X(ULong, "m")      /* unsigned long int */                                   \
  X(Int128, "n")     /* __int128 */                                            \
  X(UInt128, "o")    /* unsigned __int128 */                                   \
  X(Short, "s")      /* (signed) short */                                      \
  X(UShort, "t")     /* unsigned short */                                      \
  X(Void, "v")       /* void */                                                \
  X(WChar_S, "w")    /* wchar_t */                                             \
  X(LongLong, "x")   /* long long int */                                       \
  X(ULongLong, "y")  /* unsigned long long int */
#define CaseStmt(TyKind, Str)                                                  \
  case (BuiltinType::Kind::TyKind): {                                          \
    return Str;                                                                \
  }

  if (dyn_cast<const BuiltinType>(TypePtr)) {
    const BuiltinType *BT = dyn_cast<const BuiltinType>(TypePtr);
    const auto Kind = BT->getKind();

    switch (Kind) {
      BuiltInTyKindToString(CaseStmt) default : {
        llvm::errs() << "Unknown builtin type kind: " << Kind << "\n";
        abort();
      }
    }

#undef CaseStmt
  }

  if (dyn_cast<const PointerType>(TypePtr)) {
    const PointerType *PT = dyn_cast<const PointerType>(TypePtr);
    return "P" + mangleType(PT->getPointeeType().getTypePtr(), Typedefs);
  }
  if (dyn_cast<const ReferenceType>(TypePtr)) {
    const ReferenceType *RT = dyn_cast<const ReferenceType>(TypePtr);
    return "R" + mangleType(RT->getPointeeType().getTypePtr(), Typedefs);
  }

  /**
   * Not implemented.
   */
  llvm::errs() << "Error: unhandled type kind in mangleType: "
               << TypePtr->getTypeClass() << "\n";
  abort();
}

std::string ManglingTypeVisitor::VisitBuiltinType(const BuiltinType *BT) {
  const auto Kind = BT->getKind();
#define CaseStmt(TyKind, Str)                                                  \
  case (BuiltinType::Kind::TyKind): {                                          \
    return Str;                                                                \
  }

  switch (Kind) {
    BuiltInTyKindToString(CaseStmt) default : {
      llvm::errs() << "Unknown builtin type kind: " << Kind << "\n";
      abort();
    }
  }
#undef CaseStmt
}

} /* namespace database */
} /* namespace clang */
