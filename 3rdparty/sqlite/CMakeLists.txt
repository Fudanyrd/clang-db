# We download sqlite to source directory, to avoid
# repetitive downloads.
cmake_minimum_required(VERSION 3.15)
project(SQLite3 LANGUAGES C CXX)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# XXX: Options
option(SQLITE_ENABLE_INSTALL "Enable installation of SQLite3" ON)
option(SQLITE_BUILD_PYTHON "Enable Python bindings for SQLite3" OFF)
option(SQLITE_ENABLE_TESTS "Enable building tests for SQLite3" ${SQLITE_BUILD_PYTHON})

if (NOT SQLITE_BUILD_PYTHON AND SQLITE_ENABLE_TESTS)
  message(FATAL_ERROR "SQLITE_ENABLE_TESTS requires SQLITE_BUILD_PYTHON to be ON.")
endif()

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  message(STATUS "Using SQLite3 as a top-level project.")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
else()
  message(STATUS "Using SQLite3 as a subproject. Installation disabled.")
  set(SQLITE_ENABLE_INSTALL OFF)
endif()

if (SQLITE_BUILD_PYTHON)
  find_package(cppbind CONFIG REQUIRED)
endif()

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3.c" 
         "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3.h"
         "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3ext.h"
  COMMAND sh fetch.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Downloading and extracting SQLite amalgamation..."
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/sqlite3.c"  PROPERTIES GENERATED TRUE)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/sqlite3.h" PROPERTIES GENERATED TRUE)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/sqlite3ext.h" PROPERTIES GENERATED TRUE)
add_library(sqlite3 STATIC "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3.c" sqlite_cxx.hh cxxapi.cc)

# Installation
if(SQLITE_ENABLE_INSTALL)
  include(GNUInstallDirs)
  configure_file(config.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/sqliteConfig.cmake" @ONLY)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sqliteConfig.cmake" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/sqlite)

  message(STATUS "Library Dir: ${CMAKE_INSTALL_LIBDIR}")
  message(STATUS "Include Dir:  ${CMAKE_INSTALL_INCLUDEDIR}")

  # library
  install(TARGETS sqlite3
          EXPORT sqlite3-targets
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  # includes
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3ext.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/sqlite_cxx.hh" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

# Python binding
if (SQLITE_BUILD_PYTHON)
  # do not mark this as REQURED, because when
  # installing with `pip install .`, this package
  # can be auto-detected.
  add_definitions(-D_SQLITE_HAS_PYTHON)
  find_package(PythonExtensions CONFIG)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR} 
                      ${CPPBIND_INCLUDE_DIRS}
                      ${PYTHON_INCLUDE_DIRS})
  add_library(__init__ SHARED sqlite/python.cc)
  set_target_properties(__init__ PROPERTIES PREFIX "")
  target_link_libraries(__init__ PRIVATE sqlite3 
                        ${CPPBIND_LIBRARY} 
                        ${PYTHON_LIBRARIES})

  if (DEFINED PYTHON_SITE_PACKAGES_DIR)
    set_target_properties(__init__ PROPERTIES SUFFIX "${PYTHON_EXTENSION_MODULE_SUFFIX}")
    install(TARGETS __init__ LIBRARY DESTINATION "sqlite")
  else()
    message(WARNING "Python extension module support is not available. The Python bindings will not be installed.")
  endif()
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
endif()
