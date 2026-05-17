#include <llvm/Support/raw_ostream.h>

#include "clangdb.h"

namespace clang {
namespace database _CLANGDB_VISIBILITY {

/**
 * Simply continue, but when explaining the results,
 * we will know that this is an unknown type.
 */
static constexpr char UNKNOWN_TYPE[] = "N8typename7unknownE";

static std::string
MangleTemplateTypeParmDecl(const TemplateTypeParmDecl *TTPD) {
  if (TTPD == nullptr) {
    return "N8template8typename7defaultE";
  }
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
    } else if (auto *NTTPD = llvm::dyn_cast<NonTypeTemplateParmDecl>(Param)) {
      Ret += "N8template8decltype";
      std::string Ty = MangleType(NTTPD->getType().getTypePtr());
      if (NTTPD->isParameterPack()) {
        Ty.push_back('z');
      }
      Ret += EncodeNs(Ty);
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
  X(NullPtr, "Dn")   /* std::nullptr_t, decltype(nullptr) */                   \
  X(ULongLong, "y")  /* unsigned long long int */
#define CaseStmt(TyKind, Str)                                                  \
  case (BuiltinType::Kind::TyKind): {                                          \
    return Str;                                                                \
  }

  if (dyn_cast<const BuiltinType>(TypePtr)) {
    const BuiltinType *BT = dyn_cast<const BuiltinType>(TypePtr);
    const auto Kind = BT->getKind();

    switch (Kind) {
      BuiltInTyKindToString(CaseStmt) default : { return UNKNOWN_TYPE; }
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
  if (auto *FuncTy = dyn_cast<const FunctionProtoType>(TypePtr)) {
    const auto *FT = FuncTy;
    std::string Ret = "F";
    Ret += MangleType(FT->getReturnType().getTypePtr());
    MangleFunctionParmList(Ret, FT);
    Ret.push_back('E');
    return Ret;
  }
  if (auto *ParenTy = dyn_cast<const ParenType>(TypePtr)) {
    return MangleType(ParenTy->getInnerType().getTypePtr());
  }
  if (auto *AdjustTy = dyn_cast<const AdjustedType>(TypePtr)) {
    return MangleType(AdjustTy->getAdjustedType().getTypePtr());
  }
  if (auto *DeclTy = dyn_cast<const DecltypeType>(TypePtr)) {
    return MangleType(DeclTy->getUnderlyingType().getTypePtr());
  }
  if (auto *ConstArr = dyn_cast<const ConstantArrayType>(TypePtr)) {
    llvm::APInt Length = ConstArr->getSize();
    char buf[28];
    sprintf(buf, "A%zu", Length.getZExtValue());

    std::string Ret = const_cast<const char *>(buf);
    Ret.push_back('_');
    Ret += (MangleType(ConstArr->getElementType().getTypePtr()));
    return Ret;
  }
  if (auto *DepArr = dyn_cast<const DependentSizedArrayType>(TypePtr)) {
    /**
     * It is not possible to encode(mangle) the size information.
     * Therefore, only encodes the element type.
     */
    std::string Ret = "A_";
    Ret += MangleType(DepArr->getElementType().getTypePtr());
    return Ret;
  }
  if (auto *SubstTemplateTy =
          dyn_cast<const SubstTemplateTypeParmType>(TypePtr)) {
    return MangleType(SubstTemplateTy->getReplacementType().getTypePtr());
  }
  if (auto *DependentTy = dyn_cast<const DependentNameType>(TypePtr)) {
    std::string Ret = "N";
    Ret += MangleNestedNameSpecifier(DependentTy->getQualifier());
    Ret += EncodeNs(DependentTy->getIdentifier()->getName());
    Ret.push_back('E');
    return Ret;
  }
  if (auto *ICN = dyn_cast<const InjectedClassNameType>(TypePtr)) {
    const auto *RD = ICN->getDecl();
    return "N" + MangleRecordDecl(RD) + "E";
  }

  /**
   * Not implemented.
   */
  llvm::errs() << "Error: unhandled type kind in MangleType: "
               << TypePtr->getTypeClass() << "\n";
  return UNKNOWN_TYPE;
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
      if (LSD->getLanguage() == LinkageSpecLanguageIDs::C) {
        ret = "6extern"; /* ignore all previous namespaces. */
      } else {
        /* ignored. */
      }
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
  if (const auto *NSD = (NNS->getAsNamespace())) {
    ret += EncodeNs(NSD->getName());
  } else if (const auto *RD = (NNS->getAsRecordDecl())) {
    ret += EncodeNs(RD->getName());
  } else if (const auto *Ty = NNS->getAsType()) {
    std::string Inc = MangleType(Ty);
    assert(Inc.size() >= 2);
    ret += (Inc.back() == 'E') ? Inc.substr(1, Inc.size() - 2) : Inc;
  } else if (NNS->getKind() == NestedNameSpecifier::Global) {
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

void MangleFunctionParmList(std::string &Dest, const FunctionProtoType *Proto) {
  int NumParams = 0;
  for (auto Iter = Proto->param_type_begin(); Iter != Proto->param_type_end();
       Iter++) {
    const Type *ParamType = Iter->getTypePtr();
    Dest += MangleType(ParamType);
    NumParams += 1;
  }
  if (Proto->isVariadic()) {
    Dest += "z"; /* ... */
  } else if (NumParams == 0) {
    Dest += "v"; /* void */
  }
}

std::string EncodeFunctionName(const FunctionDecl *FD) {
  if (const auto *DGD = dyn_cast<const CXXDeductionGuideDecl>(FD)) {
    FD = DGD->getCorrespondingConstructor();
  }
  if (FD == nullptr) {
    return "0"; /* = EncodeNs("") */
  }

  const auto OpKind = FD->getOverloadedOperator();
  if (OpKind != OverloadedOperatorKind::OO_None) {
    std::string ShortName;

#define CaseStmt(Kind, ReadableName, MangledName)                              \
  case (OverloadedOperatorKind::Kind): {                                       \
    ShortName = MangledName;                                                   \
    break;                                                                     \
  }

    switch (OpKind) {
      MapOverloadedOperatorKind(CaseStmt) default : {
        llvm::errs() << "Unknown overloaded operator kind: " << OpKind << "\n";
        abort();
      }
    }
#undef CaseStmt
    return ShortName;
  }

  if (const CXXMethodDecl *Method = dyn_cast<CXXMethodDecl>(FD)) {
    if (llvm::isa<CXXConstructorDecl>(Method)) {
      return "C";
    } else if (llvm::isa<CXXDestructorDecl>(Method)) {
      return "D";
    } else if (Method->isCopyAssignmentOperator() ||
               Method->isMoveAssignmentOperator()) {
      return "aS";
    } else if (const auto *Conv = dyn_cast<CXXConversionDecl>(Method)) {
      return "cv" + MangleType(Conv->getConversionType().getTypePtr());
    }
  }

  /* Try use ->getName. this may triggers an assertion failure. */
  if (FD->getIdentifier() == nullptr) {
    llvm::errs() << "Error: unnamed function decl\n";
    FD->dump();
    abort();
  }
  return EncodeNs(FD->getName());
}

} /* namespace database _CLANGDB_VISIBILITY */
} /* namespace clang */
