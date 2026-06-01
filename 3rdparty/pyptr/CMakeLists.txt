cmake_minimum_required(VERSION 3.15)
project(pyptr LANGUAGES CXX)

find_package(cppbind CONFIG REQUIRED)
find_package(PythonExtensions CONFIG)

# the extension consists only of __init__.so
include_directories(${CPPBIND_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
add_library(__init__ SHARED pyptr/lib.cc)
target_link_libraries(__init__ PRIVATE ${CPPBIND_LIBRARY} ${PYTHON_LIBRARIES})
set_target_properties(__init__ PROPERTIES PREFIX "")
set_target_properties(__init__ PROPERTIES SUFFIX "${PYTHON_EXTENSION_MODULE_SUFFIX}")
install(TARGETS __init__ LIBRARY DESTINATION ${CMAKE_PROJECT_NAME})
