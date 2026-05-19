#include "explain.h"

namespace clang {
namespace database _CLANGDB_VISIBILITY {

std::ostream &ExplainNamespace(std::ostream &OS, std::string_view Name,
                               std::string_view Member, std::string_view Type) {
  /**
   * <h3>Interpreting a Tuple in namespace table</h3>
   *
   * @param Name: the name of current namespace.
   * Its format is concatenated length and name, e.g. `3foo3bar'
   * stands for "namespace foo::bar".
   */

  /**
   * @param Type:
   * 1. Function: it is the function's return value.
   *    See `TypeofFunctionDecl` and `TypeofFunctionTemplateDecl'.
   *
   * 2. Variable (possibly constexpr): of format (9constexpr)(6static)
   * plus type.
   *    See `TypeofVarDecl` which implements this
   *
   * 3. class/struct/enum/union: first token
   * (one of 5class/6struct/4enum/5union) indicates its type. If
   * it has base classes, the token '7virtual' will follow,
   * and the rest are encoding of its base classes, e.g.
   * "6public4base" means ": public base".
   * See `TypeofCXXRecordDecl` and `EncodeBaseClasses`.
   *
   * 4. namespace: the token "9namespace".
   */

  /**
   * @param member: depends on type of the symbol.
   * 1. Function: encoded name + (template parameter + return type
   *  if it is a template) + parameter list.
   * e.g. "2fnv" for "fn(void)"; "2fnIiiEPiRi" for
   * "fn<int,int>(int&) -> int *".
   *
   * 2. variable: a single token, e.g. "1x" for the variable "x".
   * 3. class/struct/enum/union: its name plus template parameter list.
   * 4. namespace: its name.
   */
  return OS;
}

} /* namespace database _CLANGDB_VISIBILITY */
} /* namespace clang */
