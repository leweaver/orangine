project(OeApp VERSION 1.0
        DESCRIPTION "Orangine App Library"
        LANGUAGES CXX)

configure_file (
        "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}Config.h.in"
        "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h"
)

# --------------------------------
# Library Definition
add_library(${PROJECT_NAME} STATIC
        ${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h
        resources/Win32/resource.rc
        resources/Win32/resource.aps
        resources/Win32/settings.manifest
        src/App_win32.cpp
        src/pch.h
        src/pch.cpp
        src/LogFileSink.h
        src/LogFileSink.cpp
        src/VectorLogSink.h
        src/VisualStudioLogSink.h
        src/filesinkhelper.ipp
        src/SampleScene.cpp
        src/Statics.cpp
        src/OeApp_bindings.cpp
        src/OeApp_embedded_module.cpp)

file(GLOB OeApp_HEADERS "include/OeApp/*")
list(APPEND OeApp_HEADERS "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h")

add_library(Oe::App ALIAS OeApp)

# Python Scripts Dir
include (OePython)
oe_add_scripts_dir(OeApp oe_app_bindings
        LINK_LIBRARIES Oe::Core Oe::Scripting)

# Precompiled Headers
include(PrecompiledHeader)
add_precompiled_header(${PROJECT_NAME} pch.h 
        SOURCE_CXX src/pch.cpp
        IGNORE_FILES src/OeApp_bindings.cpp
        )

target_link_libraries(${PROJECT_NAME} 
        PUBLIC OeCompilerFlags WindowsApp.lib
        PUBLIC Oe::Core
        OeScripting)

# Public headers
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )

# --------------------------------
# Install
include(OeUtils)
install_target_pdb(OeApp)

# Data and Assets
set(${PROJECT_NAME}_DATA_SOURCE_DIRS "${PROJECT_NAME}=${PROJECT_SOURCE_DIR}/data")
set(${PROJECT_NAME}_DATA_SOURCE_DIRS "${${PROJECT_NAME}_DATA_SOURCE_DIRS}" PARENT_SCOPE)
install(DIRECTORY data
        DESTINATION bin
        )

# public headers
install(
        FILES ${OeApp_HEADERS}
        DESTINATION include/${PROJECT_NAME}
)

# Config.cmake files
configure_file(OeAppConfig.cmake.in OeAppConfig.cmake @ONLY)
configure_file(OeAppConfigVersion.cmake.in OeAppConfigVersion.cmake @ONLY)
install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/OeAppConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/OeAppConfigVersion.cmake"
        DESTINATION lib/cmake/OeApp
)
