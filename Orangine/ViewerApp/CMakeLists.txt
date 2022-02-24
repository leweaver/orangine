project(ViewerApp VERSION 1.0
        DESCRIPTION "Orangine Sample GLTF Viewer Application"
        LANGUAGES CXX)

# --------------------------------
# Library Definition
add_executable(${PROJECT_NAME} WIN32
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
target_precompile_headers(${PROJECT_NAME} REUSE_FROM OeApp)

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
set(ViewerApp_THIRDPARTY_PATH ${CMAKE_SOURCE_DIR}/../thirdparty)
set(ViewerApp_SCRIPTS_PATH ${PROJECT_SOURCE_DIR}/scripts)
set(ViewerApp_DEFAULT_ASSET_PATH ${PROJECT_SOURCE_DIR}/data)
set(_ADD_CONFIG_YAML_PARTIAL_FILE "${PROJECT_BINARY_DIR}/config_yaml/viewer_app_config.yaml.partial")
configure_file("cmake/viewer_app_config.yaml.in" "${_ADD_CONFIG_YAML_PARTIAL_FILE}")
oe_target_add_config_yaml(${PROJECT_NAME} "${_ADD_CONFIG_YAML_PARTIAL_FILE}")

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
endif()