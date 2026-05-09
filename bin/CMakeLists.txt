include_directories(${CLANGDB_PRIVATE_INCLUDE_DIRS})
add_llvm_library(BuildDatabase MODULE plugin.cc PLUGIN_TOOL clang)
clang_target_link_libraries(BuildDatabase PRIVATE clangdb)
