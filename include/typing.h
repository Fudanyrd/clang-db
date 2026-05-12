#ifndef __TYPING_H__
#define __TYPING_H__ (1)

#include <map>
#include <string>

#include <clang/AST/Type.h>
#include <clang/AST/TypeVisitor.h>

#include "common.h"

namespace clang {

namespace database _CLANGDB_VISIBILITY {

std::string MangleType(const Type *TypePtr,
                       std::map<std::string, std::string> &Typedefs);

std::string MangleTemplateParameterList(const TemplateParameterList &List);

inline std::string MangleClassTemplate(ClassTemplateDecl &CTD) {
  return EncodeNs(CTD.getTemplatedDecl()->getName()) +
         MangleTemplateParameterList(*CTD.getTemplateParameters());
}

struct ManglingTypeVisitor
    : public clang::TypeVisitor<ManglingTypeVisitor, std::string> {
  std::map<std::string, std::string> &Typedefs;

  ManglingTypeVisitor(std::map<std::string, std::string> &Typedefs)
      : Typedefs(Typedefs) {}

  std::string VisitBuiltinType(const BuiltinType *BT);

  std::string VisitPointerType(const PointerType *PT) {
    return "P" + Visit(PT->getPointeeType().getTypePtr());
  }

  std::string VisitReferenceType(const ReferenceType *RT) {
    return "R" + Visit(RT->getPointeeType().getTypePtr());
  }
};

} // namespace database _CLANGDB_VISIBILITY

} /* namespace clang */

#endif /* __TYPING_H__ */
