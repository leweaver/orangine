cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(Orangine VERSION 1.0
                 DESCRIPTION "Orangine - realtime glTF rendering engine"
                 LANGUAGES CXX)

# Update module path
list(APPEND CMAKE_MODULE_PATH "${Orangine_SOURCE_DIR}/CMake;${Orangine_SOURCE_DIR}/cmake/modules")
list(APPEND CMAKE_PREFIX_PATH "${Orangine_SOURCE_DIR}/../bin/x64/${CMAKE_BUILD_TYPE}/lib/cmake")

include(OeCompilerFlags)

add_subdirectory(OeCore)
add_subdirectory(OeScripting)
add_subdirectory(ViewerApp)