/**
 * A standalone executable that builds symbol database.
 *
 * <h2>SYNOPSIS</h2>
 * dbgen [-I <include dir> | -D <defines> | -include <file>] [-o output path]
 * [-x <language>] [-std=c++] <input file 1> [rest input files...]
 *
 * <h2>Examples</h2>
 * <blockquote><pre>
 * $ cat << EOF | ./bin/dbgen -
 * > extern "C" { int f(); }
 * > EOF
 * # Namespaces
 * 6extern 1fv i
 * </pre></blockquote>
 */
#include <clangdb.h>
#include <fstream>
#include <iostream>
#include <sstream>

static const char *OutputPath = nullptr;

static clang::Language AutoDetect(const char *Path) {
  static std::string CSuffix = ".c";
  static std::set<std::string> CXXSuffixes = {".cpp", ".cxx", ".cc", ".C"};
  const auto Suffix = llvm::sys::path::extension(Path);
  std::string SuffixStr(Suffix);

  if (SuffixStr == CSuffix) {
    return clang::Language::C;
  }
  if (CXXSuffixes.count(SuffixStr)) {
    return clang::Language::CXX;
  }
  return clang::Language::Unknown;
}

static clang::Language LanguageFromString(const char *Lang) {
  static const std::map<std::string, clang::Language> LangMap = {
      {"c", clang::Language::C},
      {"c++", clang::Language::CXX},
  };

  auto Iter = LangMap.find(Lang);
  if (Iter == LangMap.end()) {
    std::cerr << "error: unknown language: " << Lang << std::endl;
    exit(1);
  }
  return Iter->second;
}

static const char *LookingAhead(int argc, char **argv, int &i, int start = 2) {
  const char *Result = argv[i] + start;
  if (Result[0] != '\0') {
    return Result;
  }
  if (i + 1 >= argc) {
    std::cerr << "error: missing argument for " << argv[i] << std::endl;
    exit(1);
  }
  return argv[++i];
}

/**
 * Std is of format r'c\d\d' or 'c++\d\d' (e.g. c++11).
 */
static void ParseLangStd(const char *Std, clang::LangOptions &LangOpts) {
#define Raise                                                                  \
  do {                                                                         \
    std::cerr << "error: invalid value in '-std=': " << Std << std::endl;      \
    exit(1);                                                                   \
  } while (0)

  if (strlen(Std) < 3) {
    Raise;
  }

  if (strncmp("c++", Std, 3) == 0) {
    int CXXStd = atoi(Std + 3);
    LangOpts.CPlusPlus = true;
    switch (CXXStd) {
    case (98): {
      break;
    }
    case (11): {
      LangOpts.CPlusPlus11 = true;
      break;
    }
    case (14): {
      LangOpts.CPlusPlus14 = true;
      break;
    }
    case (17): {
      LangOpts.CPlusPlus17 = true;
      break;
    }
    case (20): {
      LangOpts.CPlusPlus20 = true;
      break;
    }
    case (23): {
      LangOpts.CPlusPlus23 = true;
      break;
    }
    default: {
      Raise;
    }
    }

    return;
  }

  if (Std[0] == 'c') {
    int CStd = atoi(Std + 1);
    switch (CStd) {
    case (89): {
      LangOpts.C99 = true;
      break;
    }
    case (99): {
      LangOpts.C99 = true;
      break;
    }
    case (11): {
      LangOpts.C11 = true;
      break;
    }
    case (17): {
      LangOpts.C17 = true;
      break;
    }
    default: {
      Raise;
    }
    }

    return;
  }

  Raise;
#undef Raise
}

static void ArgParse(int Argc, char **Argv,
                     clang::FrontendOptions &FrontendOpts,
                     clang ::PreprocessorOptions &PreprocessorOpts,
                     clang::LangOptions &LangOpts,
                     clang::HeaderSearchOptions &HSOpts) {
  clang::Language Lang = clang::Language::Unknown;
  FrontendOpts.ProgramAction = clang::frontend::PluginAction;

  int i = 1;
  for (; i < Argc; i++) {
    const char *Arg = Argv[i];
    if (Arg[0] != '-') {
      /* Treat it as an input file. */
      FrontendOpts.Inputs.push_back(clang::FrontendInputFile(
          Arg, Lang == clang::Language::Unknown ? AutoDetect(Arg) : Lang));
      PreprocessorOpts.addRemappedFile(
          Arg, llvm::MemoryBuffer::getFile(llvm::Twine(Arg)).get().release());
      continue;
    }

    switch (Arg[1]) {
    case ('x'): {
      /* Look ahead and reset Lang. */
      const char *LangStr = LookingAhead(Argc, Argv, i);
      Lang = LanguageFromString(LangStr);
      LangOpts.CPlusPlus = (Lang == clang::Language::CXX);
      break;
    }
    case ('D'): {
      /* Look ahead and add to PreprocessorOpts. */
      const char *Define = LookingAhead(Argc, Argv, i);
      PreprocessorOpts.addMacroDef(Define);
      break;
    }
    case ('I'): {
      /* Look ahead and add to PreprocessorOpts. */
      const char *IncludeDir = LookingAhead(Argc, Argv, i);
      HSOpts.AddPath(IncludeDir, clang::frontend::Angled, false, false);
      break;
    }
    case ('o'): {
      /* Look ahead and set OutputPath. */
      OutputPath = LookingAhead(Argc, Argv, i);
      break;
    }
    case ('i'): {
      if (strncmp("-include", Arg, 8) == 0) {
        /* Look ahead and add to PreprocessorOpts. */
        const char *IncludeFile = LookingAhead(Argc, Argv, i, /* start= */ 8);
        PreprocessorOpts.Includes.push_back(IncludeFile);
      } else {
        std::cerr << "error: unknown argument: " << Arg << std::endl;
        exit(1);
      }
      break;
    }
    case ('s'): {
      if (strncmp(Arg, "-std=", 5) == 0) {
        ParseLangStd(Arg + 5, LangOpts);
      } else {
        std::cerr << "error: unknown argument: " << Arg << std::endl;
        exit(1);
      }
      break;
    }
    default: {
      std::cerr << "error: unknown argument: " << Arg << std::endl;
      exit(1);
    }
    }
  }
}

/**
 * Currently implemented synopsis:
 * dbgen <input file>
 */
int main(int argc, char **argv, char **envp) {
  std::shared_ptr<clang::CompilerInvocation> Invocation =
      std::make_shared<clang::CompilerInvocation>();
  Invocation->getTargetOpts().Triple = "x86_64-unknown-linux-gnu";
  ArgParse(argc, argv, Invocation->getFrontendOpts(),
           Invocation->getPreprocessorOpts(), Invocation->getLangOpts(),
           Invocation->getHeaderSearchOpts());

  std::unique_ptr<clang::CompilerInstance> Instance =
      std::make_unique<clang::CompilerInstance>();
  Instance->setInvocation(Invocation);
  Instance->createDiagnostics();

  std::unique_ptr<clang::database::DatabaseInterface> DB =
      std::make_unique<clang::database::InMemoryDatabase>();

  std::unique_ptr<clang::FrontendAction> Action =
      std::make_unique<clang::database::BuildDatabaseAction>(*DB);
  bool result = Instance->ExecuteAction(*Action);
  if (!result) {
    exit(1);
  }

  auto *Database = static_cast<clang::database::InMemoryDatabase *>(DB.get());
  std::cout << "# Classes" << std::endl;
  for (const auto &Tuple : Database->GetClasses()) {
    std::cout << std::get<0>(Tuple) << " ";
    std::cout << std::get<1>(Tuple) << " ";
    std::cout << std::get<2>(Tuple) << std::endl;
  }

  std::cout << "\n# Namespaces" << std::endl;
  for (const auto &Tuple : Database->GetNamespaces()) {
    std::cout << std::get<0>(Tuple) << " ";
    std::cout << std::get<1>(Tuple) << " ";
    std::cout << std::get<2>(Tuple) << std::endl;
  }

  return 0;
}
