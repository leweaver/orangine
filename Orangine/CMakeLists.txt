cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

# Hide the 'Up-to-date' install messages
set(CMAKE_INSTALL_MESSAGE LAZY)

project(Orangine VERSION 1.0
                 DESCRIPTION "Orangine - realtime glTF rendering engine"
                 LANGUAGES CXX)

# Update module path
list(APPEND CMAKE_MODULE_PATH "${Orangine_SOURCE_DIR}/CMake;${Orangine_SOURCE_DIR}/cmake/modules")

include(OeVars)
include(OeCompilerFlags)

add_subdirectory(OeCore)
add_subdirectory(OeScripting)
add_subdirectory(OeApp)
add_subdirectory(ViewerApp)