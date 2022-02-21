project(ViewerApp VERSION 1.0
        DESCRIPTION "Orangine Sample GLTF Viewer Application"
        LANGUAGES CXX)

# --------------------------------
# Configure File
set(ViewerApp_THIRDPARTY_PATH ${CMAKE_SOURCE_DIR}/../thirdparty)
configure_file (
        "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}Config.h.in"
        "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h"
)

# --------------------------------
# Library Definition
add_executable(${PROJECT_NAME} WIN32
        ${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h
        src/pch.h
        src/pch.cpp
        src/ViewerApp.cpp
        )

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Visual studio directory hint
set_property(TARGET ViewerApp PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# --------------------------------
# Precompiled Headers
include(PrecompiledHeader)
add_precompiled_header(${PROJECT_NAME} pch.h
        SOURCE_CXX src/pch.cpp)

# --------------------------------
# Dependencies

target_link_libraries(${PROJECT_NAME} PUBLIC Oe::App)

add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND "${CMAKE_COMMAND}" -E copy "${OeScripting_PYTHON_RUNTIME_DLL}" "${PROJECT_BINARY_DIR}"
                   )

# configure /data folder
oe_target_add_data_dir(${PROJECT_NAME} DEPENDENCIES Oe::App Oe::Core Oe::Scripting)

# --------------------------------
# Configuration file blending
oe_target_build_config_yaml(${PROJECT_NAME} SOURCE_TARGETS ${PROJECT_NAME} Oe::App Oe::Core Oe::Scripting)

# --------------------------------
# Installation

# Install Data and Assets
install(DIRECTORY data;${OeCore_DATA_PATH};${OeScripting_DATA_PATH};${OeApp_DATA_PATH}
        DESTINATION bin
        )

# Install PDB
include(OeUtils)
oe_target_install_pdb(ViewerApp)

# Config.cmake files
configure_file(cmake/ViewerAppConfig.cmake.in ViewerAppConfig.cmake @ONLY)
configure_file(cmake/ViewerAppConfigVersion.cmake.in ViewerAppConfigVersion.cmake @ONLY)
install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/ViewerAppConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/ViewerAppConfigVersion.cmake"
        DESTINATION lib/cmake/ViewerApp
)

if(OE_BUILD_TESTING)
    enable_testing()
end()