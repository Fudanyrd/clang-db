#include <llvm/Support/raw_ostream.h>

#include "clangdb.h"

namespace clang {
namespace database _CLANGDB_VISIBILITY {

static std::string
MangleTemplateTypeParmDecl(const TemplateTypeParmDecl *TTPD) {
  std::string ret = "N8template8typename" + EncodeNs(TTPD->getName());
  if (TTPD->isParameterPack()) {
    ret += "8decltype1z";
  }
  ret.push_back('E');
  return ret;
}

std::string MangleTemplateParameterList(const TemplateParameterList &List) {
  std::string Ret = "I";
  /**
   * Equals to `EncodeNs("template") + EncodeNs("typename")`.
   */
  static constexpr char ArgPrefix[] = "N8template8typename";
  for (auto Param : List) {
    if (auto *TTPD = llvm::dyn_cast<TemplateTypeParmDecl>(Param)) {
      Ret += MangleTemplateTypeParmDecl(TTPD);
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
    return MangleTemplateTypeParmDecl(ParamDecl);
  }
  if (dyn_cast<RecordType>(TypePtr)) {
    const RecordType *RT = dyn_cast<const RecordType>(TypePtr);
    const auto *RD = RT->getDecl();
    return "N" + MangleRecordDecl(RD) + "E";
  }
  if (dyn_cast<const ElaboratedType>(TypePtr)) {
    const ElaboratedType *ET = dyn_cast<const ElaboratedType>(TypePtr);
    return MangleType(ET->getNamedType().getTypePtr());
  }

  if (dyn_cast<const TemplateSpecializationType>(TypePtr)) {
    const auto *TST = dyn_cast<const TemplateSpecializationType>(TypePtr);
    return "N" + MangleTemplateName(TST->getTemplateName()) + "E";
  }
  if (dyn_cast<const PackExpansionType>(TypePtr)) {
    const auto *PET = dyn_cast<const PackExpansionType>(TypePtr);
    return MangleType(PET->getPattern().getTypePtr());
  }

  if (auto *UT = dyn_cast<const UsingType>(TypePtr)) {
    return MangleType(UT->getUnderlyingType().getTypePtr());
  }
  if (auto *TyDefTy = dyn_cast<const TypedefType>(TypePtr)) {
    return MangleType(TyDefTy->getDecl()->getUnderlyingType().getTypePtr());
  }
  if (auto *TyOfTy = dyn_cast<const TypeOfType>(TypePtr)) {
    return MangleType(TyOfTy->getUnmodifiedType().getTypePtr());
  }

  /**
   * Not implemented.
   */
  llvm::errs() << "Error: unhandled type kind in MangleType: "
               << TypePtr->getTypeClass() << "\n";
  abort();
}

std::string MangleNamespaceStack(const std::vector<DeclContext *> &Nesting,
                                 size_t StartIdx) {
  std::string ret = "";

  /* Top is C Linkage. */
  if (Nesting.empty()) {
    return ret;
  }
  if (const auto *LSD = dyn_cast<LinkageSpecDecl>(Nesting.back())) {
    if (LSD->getLanguage() == LinkageSpecLanguageIDs::C) {
      return "6extern";
    }
  }

  const size_t Depth = Nesting.size();
  for (; StartIdx < Depth; StartIdx += 1) {
    const auto *Ptr = Nesting[StartIdx];
    if (auto *ND = dyn_cast<const NamespaceDecl>(Ptr)) {
      ret += EncodeNs(ND->getName());
    } else if (auto *RD = dyn_cast<const CXXRecordDecl>(Ptr)) {
      ret += EncodeNs(RD->getName());
    } else if (auto *LSD = dyn_cast<const LinkageSpecDecl>(Ptr)) {
      /* Ignored. */
    } else {
      llvm::errs() << "Unexpected decl context in namespace stack: "
                   << Ptr->getDeclKindName() << "\n";
      assert(0);
    }
  }

  return ret;
}

std::string MangleRecordDecl(const TagDecl *RD) {
  std::vector<DeclContext *> Nesting;
  const DeclContext *Iter = RD->getDeclContext();
  while (Iter) {
    if (dyn_cast<TranslationUnitDecl>(Iter)) {
      /* Top level of AST. */
      break;
    }
    Nesting.push_back(const_cast<DeclContext *>(Iter));
    Iter = Iter->getParent();
  }

  std::reverse(Nesting.begin(), Nesting.end());
  std::string ret = MangleNamespaceStack(Nesting);
  ret += EncodeNs(RD->getName());
  return ret;
}

std::string MangleNestedNameSpecifier(const NestedNameSpecifier *NNS) {
  std::string ret;
  if (const auto *NSD = dyn_cast<NamespaceDecl>(NNS->getAsNamespace())) {
    ret += EncodeNs(NSD->getName());
  } else if (const auto *RD = dyn_cast<RecordDecl>(NNS->getAsRecordDecl())) {
    ret += EncodeNs(RD->getName());
  } else {
    llvm::errs()
        << "Error: unexpected decl context in MangleNestedNameSpecifier: "
        << NNS->getKind() << "\n";
    abort();
  }
  return ret;
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
