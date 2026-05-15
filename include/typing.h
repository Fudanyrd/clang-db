#ifndef __TYPING_H__
#define __TYPING_H__ (1)

#include <map>
#include <string>

#include <clang/AST/Type.h>
#include <clang/AST/TypeVisitor.h>

#include "common.h"

namespace clang {

namespace database _CLANGDB_VISIBILITY {

/**
 * <h1>Specification Change</h1>
 * The `member` column of a fuction method shall have only
 * one of the following formats:
 * <ul>
 *   <li>Plain: length plus name, e.g. 2fn</li>
 *   <li>Constructor: C</li>
 *   <li>Destructor: D</li>
 *   <li>Overloaded operators: a short string specified in
 *     `MapOverloadedOperatorKind` below</li>
 * </ul>
 *
 * It correctly handles overloaded operators, constructors,
 * and destructors. User should use this instead of simply
 * call `FD->getName()'.
 *
 * This encodes the function name <b>without</b> appending the
 * types of the parameter list.
 */
std::string EncodeFunctionName(const FunctionDecl *FD);
std::string MangleType(const Type *TypePtr);

/**
 * @return the wholename (i.e. including the namespaces) of RD.
 */
std::string MangleRecordDecl(const TagDecl *RD);
std::string MangleNestedNameSpecifier(const NestedNameSpecifier *NNS);

/**
 * Mangle each parameter type and append the result to `Dest`.
 */
void MangleFunctionParmList(std::string &Dest, const FunctionProtoType *Proto);

std::string MangleTemplateParameterList(const TemplateParameterList &List);

std::string MangleNamespaceStack(const std::vector<DeclContext *> &Nesting,
                                 size_t StartIdx = 0);

inline std::string MangleClassTemplate(ClassTemplateDecl &CTD) {
  return EncodeNs(CTD.getTemplatedDecl()->getName()) +
         MangleTemplateParameterList(*CTD.getTemplateParameters());
}

inline std::string MangleFunctionTemplate(FunctionTemplateDecl &FTD) {
  FunctionDecl *Fn = FTD.getTemplatedDecl();
  return EncodeFunctionName(Fn) +
         MangleTemplateParameterList(*FTD.getTemplateParameters()) +
         MangleType(Fn->getReturnType().getTypePtr());
}

inline std::string MangleTemplateName(const TemplateName &TN) {
  TemplateDecl *TD = TN.getAsTemplateDecl();
  if (auto *CTD = llvm::dyn_cast<ClassTemplateDecl>(TD)) {
    return MangleClassTemplate(*CTD);
  } else if (auto *FTD = llvm::dyn_cast<FunctionTemplateDecl>(TD)) {
    return MangleFunctionTemplate(*FTD);
  } else {
    return "8typename7unknown";
  }
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

/**
 * Hint: clang::OverloadedOperatorKind.
 *
 * Each entry is of format 'OperatorKind', human
 * readable name, and the mangled name for the operator.
 */
#define MapOverloadedOperatorKind(X)                                           \
  X(OO_AmpEqual, "&=", "aN")                                                   \
  X(OO_Equal, "=", "aS")                                                       \
  X(OO_SlashEqual, "/=", "dV")                                                 \
  X(OO_CaretEqual, "^=", "eO")                                                 \
  X(OO_LessLessEqual, "<<=", "lS")                                             \
  X(OO_MinusEqual, "-=", "mI")                                                 \
  X(OO_StarEqual, "*=", "mL")                                                  \
  X(OO_PipeEqual, "|=", "oR")                                                  \
  X(OO_PlusEqual, "+=", "pL")                                                  \
  X(OO_PercentEqual, "%=", "rM")                                               \
  X(OO_GreaterGreaterEqual, ">>=", "rS")                                       \
  X(OO_Amp, "&", "an")                                                         \
  X(OO_Tilde, "~", "co")                                                       \
  X(OO_Slash, "/", "dv")                                                       \
  X(OO_Caret, "^", "eo")                                                       \
  X(OO_EqualEqual, "==", "eq")                                                 \
  X(OO_GreaterEqual, ">=", "ge")                                               \
  X(OO_Greater, ">", "gt")                                                     \
  X(OO_LessEqual, "<=", "le")                                                  \
  X(OO_LessLess, "<<", "ls")                                                   \
  X(OO_Less, "<", "ls")                                                        \
  X(OO_Minus, "-", "mi")                                                       \
  X(OO_Star, "*", "ml")                                                        \
  X(OO_MinusMinus, "--", "mm")                                                 \
  X(OO_ExclaimEqual, "!=", "ne")                                               \
  X(OO_Exclaim, "!", "ng")                                                     \
  X(OO_Pipe, "|", "or")                                                        \
  X(OO_Plus, "+", "pl")                                                        \
  X(OO_PlusPlus, "++", "pp")                                                   \
  X(OO_Percent, "%", "ps")                                                     \
  X(OO_Call, "()", "cl")                                                       \
  X(OO_Subscript, "[]", "ix")                                                  \
  X(OO_New, "new", "nw")                                                       \
  X(OO_Delete, "delete", "dl")                                                 \
  X(OO_Array_New, "new[]", "na")                                               \
  X(OO_Array_Delete, "delete[]", "da")                                         \
  X(OO_GreaterGreater, ">>", "rs")

} /* namespace clang */

#endif /* __TYPING_H__ */
