#ifndef __AST_H__
#define __AST_H__ (1)

#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/PartialDiagnostic.h>
#include <clang/Frontend/FrontendAction.h>
#include <memory>
#include <stack>

#include "storage.h"
#include "typing.h"

namespace clang {

namespace database _CLANGDB_VISIBILITY {

/**
 * <blockquote><pre>
 * class A { };
 * class B { };
 * // type of class C:
 * // class::virtual::public::virtual::A::protected::B
 * // the first `virtual' indicate start of base classes.
 * class C : public virtual A, protected B { };
 * </pre></blockquote>
 */
void EncodeBaseClasses(std::string &Dest, const CXXRecordDecl *RD);

struct BuildVisitor;
struct BuildDatabaseConsumer;
struct ClassDeclVisitor; /* internal use. */

/**
 * <h3>ReadMe - Type Column Spec</h3>
 *
 * The "type" column for a C++ record is "5class" or "6struct".
 */
inline const char *TypeofCXXRecord(const CXXRecordDecl *RD) {
  const char *Type;
  if (RD->isStruct()) {
    Type = "6struct";
  } else if (RD->isClass()) {
    Type = "5class";
  } else if (RD->isUnion()) {
    Type = "5union";
  } else if (RD->isEnum()) {
    Type = "4enum";
  } else {
    llvm_unreachable("Unknown CXXRecordDecl kind");
  }
  return Type;
}

inline std::string TypeofCXXRecordDecl(const CXXRecordDecl *RD) {
  std::string Ret = TypeofCXXRecord(RD);
  EncodeBaseClasses(Ret, RD);
  return Ret;
}

inline const char *EncodeAccessSpecifier(AccessSpecifier Access) {
  switch (Access) {
  case AS_public:
    return "6public";
  case AS_protected:
    return "9protected";
  case AS_private:
    return "7private";
  }
  /* AS_none: */
  llvm_unreachable("Unknown AccessSpecifier");
}

inline std::string TypeofLocalCXXRecordDecl(CXXRecordDecl *RD,
                                            AccessSpecifier Access) {
  const char *Type = TypeofCXXRecord(RD);
  std::string Ret = Type;
  Ret += EncodeAccessSpecifier(Access);
  EncodeBaseClasses(Ret, RD);
  return Ret;
}

std::string TypeofClassMember(FieldDecl *Value, AccessSpecifier Access);

/**
 * @param Value: the variable declaration to be mangled.
 * @param Access: caller should set it to none when it is not
 * declared in a CXXRecordDecl, or it is a static data member. Otherwise, it
 * should be set to the access specifier of the declaration.
 */
std::string TypeofVarDecl(VarDecl *Value, AccessSpecifier Access);

std::string TypeofLocalCXXMethodDecl(CXXMethodDecl *Method,
                                     AccessSpecifier Access);

/**
 * The visitor for building the database. It traverses the AST and
 * fills the database tables, as specified in ReadMe.
 */
struct BuildVisitor : public RecursiveASTVisitor<BuildVisitor> {
  BuildVisitor(DatabaseInterface *DB) : Database(DB) {}
  bool TraverseTranslationUnitDecl(TranslationUnitDecl *TU);

private:
  DatabaseInterface *Database;
};

struct BuildDatabaseConsumer : public ASTConsumer {
  DatabaseInterface *Database;
  BuildDatabaseConsumer(DatabaseInterface *Database) : Database(Database) {}

  void HandleTranslationUnit(ASTContext &Ctx) override {
    BuildVisitor Visitor(this->Database);
    Visitor.TraverseDecl(Ctx.getTranslationUnitDecl());
  }
};

struct BuildDatabaseAction : public PluginASTAction {
  DatabaseInterface *DB;

  BuildDatabaseAction(void) : DB(nullptr) {}
  BuildDatabaseAction(DatabaseInterface &Database) : DB(&Database) {}

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override {
    return std::make_unique<BuildDatabaseConsumer>(this->DB);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    DB = new InMemoryDatabase();
    return true;
  }
};

} /* namespace database _CLANGDB_VISIBILITY */

} /* namespace clang */

#endif /* __AST_H__ */
