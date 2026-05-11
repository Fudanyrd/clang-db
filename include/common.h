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

} /* namespace database _CLANGDB_VISIBILITY */
} /* namespace clang */

#endif /* __COMMON_H__ */
