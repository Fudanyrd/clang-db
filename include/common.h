#ifndef __COMMON_H__
#define __COMMON_H__ (1)

#if !defined CLANGDB_VISIBILITY
#define CLANGDB_VISIBILITY "default"
#endif /* CLANGDB_VISIBILITY */

#define _CLANGDB_VISIBILITY __attribute__((visibility(CLANGDB_VISIBILITY)))

#if defined _GLIBCXX_ASSERTIONS
#define clangdb_check_internal(expr) assert(expr)
#else /* !defined _GLIBCXX_ASSERTIONS */
#define clangdb_check_internal(expr) ((void)(expr))
#endif /* _GLIBCXX_ASSERTIONS */

#if !defined likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif /* !defined likely */

#if !defined unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif /* !defined unlikely */

#include <string>

#include <llvm/ADT/StringRef.h>

namespace clang {
namespace database _CLANGDB_VISIBILITY {

/**
 * Maximum recursion depth when traversing the AST. This is to prevent infinite
 * recursion caused by cyclic references in the AST. The value is chosen
 * arbitrarily, and can be adjusted if necessary.
 */
constexpr int MAX_RECURSION_DEPTH = 1024;

/**
 * Namespace/class encoding.
 */
inline ::std::string EncodeNs(const ::std::string &Namespace) {
  return ::std::to_string(Namespace.size()) + Namespace;
}

inline ::std::string EncodeNs(llvm::StringRef Namespace) {
  return ::std::to_string(Namespace.size()) + Namespace.str();
}

/**
 * Only used by storage.cc, but also exposed for testing.
 */
#define DefNormalizePath                                                       \
  static std::vector<char> NormalizePathName(const std::string &Path) {        \
    if (Path.empty()) {                                                        \
      return {0x0};                                                            \
    }                                                                          \
    assert(Path.back() != '/'); /* not a directory. */                         \
                                                                               \
    int Length = Path.size();                                                  \
    int ResultLength = 0;                                                      \
    std::vector<std::pair<int, int>> RangesToReplace;                          \
    RangesToReplace.reserve(8);                                                \
                                                                               \
    bool isAbsolute = Path[0] == '/';                                          \
                                                                               \
    /* split by '/' */                                                         \
    int i = 0;                                                                 \
    for (;;) {                                                                 \
      while (i < Length && Path[i] == '/') {                                   \
        i++;                                                                   \
      }                                                                        \
      if (i >= Length) {                                                       \
        break;                                                                 \
      }                                                                        \
      int Start = i;                                                           \
      int End = i + 1;                                                         \
      while (End < Length && Path[End] != '/') {                               \
        End++;                                                                 \
      }                                                                        \
                                                                               \
      /* Check for '..' */                                                     \
      if (End - Start == 2 && Path[Start] == '.' && Path[Start + 1] == '.') {  \
        if (!RangesToReplace.empty()) {                                        \
          RangesToReplace.pop_back();                                          \
        }                                                                      \
      } else if (End - Start == 1 && Path[Start] == '.') {                     \
        /* Check for '.', do nothing */                                        \
      } else {                                                                 \
        /* Normal path component. */                                           \
        RangesToReplace.emplace_back(Start, End);                              \
      }                                                                        \
      ResultLength += (End - Start); /* Not including '/' for now. */          \
      i = End + 1;                                                             \
    }                                                                          \
                                                                               \
    /* Number of '/' plus one (null term) */                                   \
    ResultLength += (isAbsolute ? 1 : 0) + RangesToReplace.size();             \
                                                                               \
    std::vector<char> NormalizedPath(ResultLength);                            \
    char *Dest = NormalizedPath.data();                                        \
    const char *Src = Path.data();                                             \
    if (isAbsolute) {                                                          \
      *Dest = '/';                                                             \
      Dest++;                                                                  \
    }                                                                          \
    for (const auto &[Start, End] : RangesToReplace) {                         \
      std::copy(Src + Start, Src + End, Dest);                                 \
      Dest += (End - Start);                                                   \
      *Dest = '/';                                                             \
      Dest++;                                                                  \
    }                                                                          \
    Dest--;                                                                    \
    *Dest = '\0'; /* Erase trailing '/' */                                     \
    return NormalizedPath;                                                     \
  }

} /* namespace database _CLANGDB_VISIBILITY */
} /* namespace clang */

#endif /* __COMMON_H__ */
