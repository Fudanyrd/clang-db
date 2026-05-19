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
    Dest += EncodeAccessSpecifier(Iter->getAccessSpecifier());
    Dest += (Iter->isVirtual() ? "7virtual" : "");
    if (Base != nullptr) {
      Dest += EncodeNs("N" + MangleRecordDecl(Base) + "E");
    } else {
      Dest += EncodeNs(MangleType(Iter->getType().getTypePtr()));
    }
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

/**
 * Example usage:
 * clang -cc1 -load /path/to/this/plugin.so -plugin build-db
 * -plugin-arg-build-db -o=output.db
 */
bool BuildDatabaseAction::ParseArgs(const CompilerInstance &CI,
                                    const std::vector<std::string> &args) {
  bool PrintHelp = false;
  const char *OFile = "";

  for (const auto &Arg : args) {
    if (Arg == "-h" || Arg == "--help") {
      PrintHelp = true;
    } else if (strncmp(Arg.c_str(), "-o=", 3) == 0) {
      OFile = Arg.substr(3).c_str();
    } else {
      llvm::errs() << "Unknown argument: " << Arg << "\n";
      return false;
    }
  }

  if (PrintHelp) {
    llvm::errs()
        << "Usage: clang-db [options]\n"
           "Options:\n"
           "  -o=<output_file>   Specify the output file for the database\n"
           "  -h, --help         Show this help message\n";
    return false;
  }

  if (*OFile == 0x0) {
    llvm::errs() << "Output file not specified. Use in-memory database.\n";
    this->DB = QualPtr(new InMemoryDatabase());
  } else {
    SqliteDatabase *DBInstance = new SqliteDatabase(OFile);
    DB = QualPtr(DBInstance);

    if (!(*DBInstance)) {
      llvm::errs() << "Failed to open database file: " << OFile << "\n";
      delete DBInstance;
      return false;
    }
  }

  DB.set<0>(true); /* should delete the database in destructor */
  return true;
}

} /* namespace database _CLANGDB_VISIBILITY */
} /* namespace clang */
