include_directories(
  ${PROJECT_SOURCE_DIR}/3rdparty/gtest/googletest/include
  ${CLANGDB_PRIVATE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE CLANGDB_TEST_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*_test.cc")

add_custom_target(run-tests 
  COMMAND echo "Running all tests...")

if(CLANGDB_BUILD_UNITTESTS)
  foreach(source_file ${CLANGDB_TEST_SRC})
    get_filename_component(filename ${source_file} NAME)
    string(REPLACE ".cc" "" testname ${filename})

    add_executable(${testname} ${source_file})
    target_link_libraries(${testname} clangdb-aux clangdb sqlite3 gtest gmock_main clang clang-cpp LLVM)

    add_dependencies(run-tests run-${testname})
    add_custom_target("run-${testname}" COMMAND "${CMAKE_CURRENT_BINARY_DIR}/${testname}"
                      DEPENDS ${testname})
    set_target_properties(${testname}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
            COMMAND ${testname}
            )
  endforeach()
else()
endif()

