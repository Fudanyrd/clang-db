#include "clangdb.h"
#include "context.h"
#include <iostream>
#include <llvm/Support/Casting.h>

namespace clang {
namespace database _CLANGDB_VISIBILITY {

void EncodeBaseClasses(std::string &Dest, const CXXRecordDecl *RD) {
  RD = RD->getDefinition();
  if (RD == nullptr) {
    return;
  }

  const auto &Bases = RD->bases();
  if (Bases.empty()) {
    return;
  }
  Dest += "7virtual";
  for (auto Iter = Bases.begin(); Iter != Bases.end(); Iter++) {
    const CXXRecordDecl *Base = Iter->getType()->getAsCXXRecordDecl();
    if (!Base) {
      continue;
    }
    Dest += EncodeAccessSpecifier(Iter->getAccessSpecifier());
    Dest += (Iter->isVirtual() ? "7virtual" : "");
    Dest += EncodeNs("N" + MangleRecordDecl(Base) + "E");
  }
}

std::string TypeofLocalCXXMethodDecl(CXXMethodDecl *Method,
                                     AccessSpecifier Access) {
  std::string MangledRetTy = MangleType(Method->getReturnType().getTypePtr());
  std::string Ret = Method->isStatic() ? "6static" : "";
  Ret += (Method->isConst() ? "5const" : "");
  Ret += (Method->isPureVirtual() ? "7virtual" : "");
  Ret += EncodeAccessSpecifier(Access);
  Ret += EncodeNs(MangledRetTy);
  return Ret;
}

std::string TypeofClassMember(FieldDecl *Value, AccessSpecifier Access) {
  /**
   * return format: mangled "(static::)(const::)(mutable::)mangled(type)"
   */
  std::string MangledTy = MangleType(Value->getType().getTypePtr());
  std::string Ret = Value->isMutable() ? "7mutable" : "";
  Ret += EncodeAccessSpecifier(Access);
  Ret += EncodeNs(MangledTy);
  return Ret;
}

std::string TypeofVarDecl(VarDecl *Value, AccessSpecifier Access) {
  std::string MangledTy = MangleType(Value->getType().getTypePtr());
  std::string Ret = Value->isStaticDataMember() ? "6static" : "";
  Ret +=
      (Access == AccessSpecifier::AS_none) ? "" : EncodeAccessSpecifier(Access);
  Ret += (Value->isConstexpr()) ? "9constexpr" : "";
  Ret += EncodeNs(MangledTy);
  return Ret;
}

bool BuildVisitor::TraverseTranslationUnitDecl(TranslationUnitDecl *TU) {
  DatabaseContext Ctx(*Database);
  Ctx.VisitTranslationUnitDecl(TU);
  return true;
}

} /* namespace database _CLANGDB_VISIBILITY */
} /* namespace clang */
