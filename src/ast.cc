#include "clangdb.h"
#include <iostream>

namespace clang {
namespace database _CLANGDB_VISIBILITY {

std::string BuildVisitorContext::CurrentNamespace() const {
  std::string ret = "";
  size_t Depth = NamespaceStack.size();

  for (size_t i = 1 /* skip translation unit */; i < Depth; i++) {
    auto *ND = llvm::dyn_cast<NamespaceDecl>(NamespaceStack[i]);
    if (!ND) {
      continue;
    }
    ret += EncodeNs(ND->getName());
  }

  return ret;
}

bool BuildVisitor::TraverseNamespaceDecl(NamespaceDecl *ND) {
  DeclContext *CurNamespace = Context.CurrentDeclContext();
  if (CurNamespace == ND->getParent()) {
    Context.EnterNamespace(ND);
  } else {
    /**
     * By default, the `RecursiveASTVisitor` traverse in preorder.
     * This means `ND` is lateral to `CurNamespace`, and we need to
     * pop the stack until we find the common ancestor of `ND` and
     * `CurNamespace`, then push `ND` into the stack.
     */
    while (CurNamespace != ND->getParent()) {
      CurNamespace = CurNamespace->getParent();
      Context.LeaveNamespaceOrClass();
    }
    Context.EnterNamespace(ND);
  }

  return RecursiveASTVisitor<BuildVisitor>::TraverseNamespaceDecl(ND);
}

} /* namespace database _CLANGDB_VISIBILITY */
} /* namespace clang */
