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
        src/Game.h
        src/Game.cpp
        src/LogFileSink.h
        src/LogFileSink.cpp
        src/VectorLogSink.h
        src/VectorLogSink.cpp
        src/filesinkhelper.ipp)

file(GLOB OeApp_HEADERS "include/OeApp/*")
list(APPEND OeApp_HEADERS "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h")

set_target_properties(OeApp PROPERTIES PUBLIC_HEADER "${OeApp_HEADERS}")
add_library(Oe::App ALIAS OeApp)

# Precompiled Headers
include(PrecompiledHeader)
add_precompiled_header(${PROJECT_NAME} pch.h 
        SOURCE_CXX src/pch.cpp)

target_link_libraries(${PROJECT_NAME} 
        PUBLIC OeCompilerFlags WindowsApp.lib
        PUBLIC Oe::Core Oe::Scripting)

# Public headers
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )

# --------------------------------
# Install
install(TARGETS ${PROJECT_NAME}
        EXPORT OeAppTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
        )

include(OeUtils)
install_target_pdb(OeApp)

install(
        EXPORT OeAppTargets
        FILE OeAppTargets.cmake
        NAMESPACE Oe::
        DESTINATION lib/cmake/${PROJECT_NAME}
        )

# Data and Assets
set(${PROJECT_NAME}_DATA_SOURCE_DIR "${PROJECT_SOURCE_DIR}/data" PARENT_SCOPE)
install(DIRECTORY data
        DESTINATION bin
        )

# Config.cmake files
configure_file(OeAppConfig.cmake.in OeAppConfig.cmake @ONLY)
configure_file(OeAppConfigVersion.cmake.in OeAppConfigVersion.cmake @ONLY)
install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/OeAppConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/OeAppConfigVersion.cmake"
        DESTINATION lib/cmake/OeApp
)
