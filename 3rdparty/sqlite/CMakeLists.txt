# We download sqlite to source directory, to avoid
# repetitive downloads.
cmake_minimum_required(VERSION 3.15)
project(SQLite3 LANGUAGES C CXX)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# XXX: Options
option(SQLITE_ENABLE_INSTALL "Enable installation of SQLite3" ON)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  message(STATUS "Using SQLite3 as a top-level project.")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
else()
  message(STATUS "Using SQLite3 as a subproject. Installation disabled.")
  set(SQLITE_ENABLE_INSTALL OFF)
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
