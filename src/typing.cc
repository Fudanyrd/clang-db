#include <llvm/Support/raw_ostream.h>

#include "clangdb.h"

namespace clang {
namespace database _CLANGDB_VISIBILITY {

std::string MangleTemplateParameterList(const TemplateParameterList &List) {
  std::string Ret = "I";
  /**
   * Equals to `EncodeNs("template") + EncodeNs("typename")`.
   */
  static constexpr char ArgPrefix[] = "N8template8typename";
  for (auto Param : List) {
    if (auto *TTPD = llvm::dyn_cast<TemplateTypeParmDecl>(Param)) {
      Ret += ArgPrefix;
      Ret += EncodeNs(TTPD->getName());
      Ret.push_back('E');
    }
  }
  Ret.push_back('E');
  return Ret;
}

std::string MangleType(const Type *TypePtr) {
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
    return "P" + MangleType(PT->getPointeeType().getTypePtr());
  }
  if (dyn_cast<const RValueReferenceType>(TypePtr)) {
    const ReferenceType *RT = dyn_cast<const ReferenceType>(TypePtr);
    return "O" + MangleType(RT->getPointeeType().getTypePtr());
  }
  if (dyn_cast<const LValueReferenceType>(TypePtr)) {
    const ReferenceType *RT = dyn_cast<const ReferenceType>(TypePtr);
    return "R" + MangleType(RT->getPointeeType().getTypePtr());
  }
  if (dyn_cast<const TemplateTypeParmType>(TypePtr)) {
    const TemplateTypeParmType *TTP =
        dyn_cast<const TemplateTypeParmType>(TypePtr);
    const auto *ParamDecl = TTP->getDecl();
    std::string ret = "N8template8typename" + EncodeNs(ParamDecl->getName());
    ret.push_back('E');
    return ret;
  }

  /**
   * Not implemented.
   */
  llvm::errs() << "Error: unhandled type kind in MangleType: "
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

} /* namespace database _CLANGDB_VISIBILITY */
} /* namespace clang */
