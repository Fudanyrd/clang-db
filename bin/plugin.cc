/**
 * Register `clang::database::BuildDatabaseAction` as a clang plugin.
 */

#include <clang/Frontend/FrontendPluginRegistry.h>

#include "clangdb.h"

using namespace clang;
static FrontendPluginRegistry::Add<clang::database::BuildDatabaseAction>
X("build-db", "build symbol database");
