#ifndef __COMMON_H__
#define __COMMON_H__ (1)


#if !defined CLANGDB_VISIBILITY
#define CLANGDB_VISIBILITY "default"
#endif /* CLANGDB_VISIBILITY */

#define _CLANGDB_VISIBILITY __attribute__((visibility(CLANGDB_VISIBILITY)))

#if defined _GLIBCXX_ASSERTIONS
#define clangdb_check_internal(expr) assert(expr)
#else /* !defined _GLIBCXX_ASSERTIONS */
#define clangdb_check_internal(expr) ((void) (expr))
#endif /* _GLIBCXX_ASSERTIONS */

#endif /* __COMMON_H__ */
