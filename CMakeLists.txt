cmake_minimum_required(VERSION 3.22)
project(clang-db)

# XXX: options
option(CLANGDB_BUILD_UNITTESTS "Build unit tests" ON)

if(DEFINED CMAKE_CXX_STANDARD)
  message(STATUS "C++ standard already set to ${CMAKE_CXX_STANDARD}")
else()
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# unit tests
if (CLANGDB_BUILD_UNITTESTS)
  add_subdirectory(test)
endif()
