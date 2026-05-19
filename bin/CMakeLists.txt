include_directories(${CLANGDB_PRIVATE_INCLUDE_DIRS})
add_llvm_library(BuildDatabase MODULE plugin.cc PLUGIN_TOOL clang)
target_link_libraries(BuildDatabase PRIVATE clangdb sqlite3)

add_executable(dbgen dbgen.cc)
target_link_libraries(dbgen PRIVATE clangdb clang clang-cpp LLVM sqlite3)

add_executable(retrieve retrieve.cc)
target_link_libraries(retrieve PRIVATE clangdb clang clang-cpp LLVM sqlite3)
