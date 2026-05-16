cmake_minimum_required(VERSION 3.22)
project(clang-db)

# XXX: options
option(CLANGDB_BUILD_UNITTESTS "Build unit tests" ON)
option(CLANGDB_BUILD_TOOLS "Build tools" ON)
option(CLANGDB_ENABLE_ASSERTIONS "Enable assertions in the code" ON)

if(DEFINED CMAKE_CXX_STANDARD)
  message(STATUS "C++ standard already set to ${CMAKE_CXX_STANDARD}")
else()
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-fno-rtti -fno-exceptions ${CMAKE_CXX_FLAGS}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (CLANGDB_ENABLE_ASSERTIONS)
  add_definitions(-D_GLIBCXX_ASSERTIONS)
endif()

find_package(Clang REQUIRED)
if(DEFINED CLANG_INCLUDE_DIRS)
  message(STATUS "Clang Include Directories: ${CLANG_INCLUDE_DIRS}")
else()
  message(FATAL_ERROR "Clang include directories not found.")
endif()

include_directories(${CLANG_INCLUDE_DIRS})

# Generate amalgamation header.
add_subdirectory(include)

# third party
add_subdirectory(3rdparty)

# library
set(CLANGDB_PRIVATE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(src)

# tools.
if(CLANGDB_BUILD_TOOLS)
  find_package(LLVM REQUIRED CONFIG)
  find_package(Clang REQUIRED CONFIG)
  include(AddLLVM)
  include("${CLANG_CMAKE_DIR}/AddClang.cmake")
  add_subdirectory(bin) # executables, plugins, etc.
endif()

# unit tests
if (CLANGDB_BUILD_UNITTESTS)
  add_subdirectory(test)
endif()

# code formatting.
add_custom_target(format
  COMMAND sh script/format.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Running clang-format on source files..."
)
add_custom_target(check-format
  COMMAND sh script/ci-format.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Checking code format with clang-format..."
)
