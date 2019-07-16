project(OeScripting VERSION 1.0
        DESCRIPTION "Orangine Scripting Library"
        LANGUAGES CXX)

configure_file (
        "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}Config.h.in"
        "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h"
)

# Library Definition
add_library(${PROJECT_NAME} STATIC
        src/pch.h
        src/pch.cpp
        src/Entity_scripting_manager.h
        src/Entity_scripting_manager.cpp
        src/Script_component.cpp
        src/Script_runtime_data.h)

file(GLOB OeScripting_HEADERS "include/OeScripting/*")
list(APPEND OeScripting_HEADERS "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h")

set_target_properties(OeScripting PROPERTIES PUBLIC_HEADER "${OeScripting_HEADERS}")
add_library(Oe::Scripting ALIAS OeScripting)

# Precompiled Headers
include(PrecompiledHeader)
add_precompiled_header(${PROJECT_NAME} pch.h SOURCE_CXX src/pch.cpp)

target_link_libraries(${PROJECT_NAME}
        PUBLIC OeCompilerFlags
        PRIVATE Oe::Core)

# Public headers
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )

# Python
find_package(Python3 COMPONENTS Development)
target_include_directories(${PROJECT_NAME}
        PUBLIC ${PYTHON_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC Python3::Python)

# PyBind
set(PYBIND11_CPP_STANDARD /std:c++17)
find_package(pybind11 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE pybind11::embed)

# Install
install(TARGETS ${PROJECT_NAME}
        EXPORT OeScriptingTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
        )

# Install Python binaries
install(FILES "${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}_d.dll"
        CONFIGURATIONS Debug
        DESTINATION bin)
install(FILES "${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.dll"
        CONFIGURATIONS Release
        DESTINATION bin)

include(OeUtils)
install_target_pdb(OeScripting)

install(
        EXPORT OeScriptingTargets
        FILE OeScriptingTargets.cmake
        NAMESPACE Oe::
        DESTINATION lib/cmake/${PROJECT_NAME}
)

# Data and Assets
set(${PROJECT_NAME}_DATA_SOURCE_DIR "${PROJECT_SOURCE_DIR}/data" PARENT_SCOPE)
install(DIRECTORY data
        DESTINATION bin
        )

# Install Python binaries
install(FILES "${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}_d.dll"
        CONFIGURATIONS Debug
        DESTINATION bin)
install(FILES "${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.dll"
        CONFIGURATIONS Release
        DESTINATION bin)

# Config.cmake files
configure_file(OeScriptingConfig.cmake.in OeScriptingConfig.cmake @ONLY)
configure_file(OeScriptingConfigVersion.cmake.in OeScriptingConfigVersion.cmake @ONLY)
install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/OeScriptingConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/OeScriptingConfigVersion.cmake"
        DESTINATION lib/cmake/OeScripting
)