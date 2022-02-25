project(ViewerApp VERSION 1.0
        DESCRIPTION "Orangine Sample GLTF Viewer Application"
        LANGUAGES CXX)

#####
# Library Definition
#####
add_executable(${PROJECT_NAME} WIN32
        src/ViewerApp.cpp
        )

# Visual studio directory hint
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

#####
# Dependencies
#####

target_link_libraries(${PROJECT_NAME} PUBLIC Oe::App)

#####
# Testing
#####
if(OE_BUILD_TESTING)
    enable_testing()
endif()

#####
# Orangine module configuration
#####

# configure /cmake folder and src/{PROJECT_NAME}Config.h.in file
oe_target_configure_cmake_dir(${PROJECT_NAME})

# configure /data folder
oe_target_add_data_dir(${PROJECT_NAME} DEPENDENCIES Oe::App Oe::Core Oe::Scripting)

# configure /scripts folder
oe_target_add_scripts_dir(${PROJECT_NAME} HAS_SCRIPTS)

# Precompiled Headers
target_precompile_headers(${PROJECT_NAME} REUSE_FROM OeApp)

# Make sure to include PDB files when installing in debug mode
oe_target_install_pdb(${PROJECT_NAME})

#####
# Configuration file blending
#####
set(ViewerApp_THIRDPARTY_PATH ${CMAKE_SOURCE_DIR}/../thirdparty)
set(ViewerApp_DEFAULT_ASSET_PATH ${PROJECT_SOURCE_DIR}/data)
set(_ADD_CONFIG_YAML_PARTIAL_FILE "${PROJECT_BINARY_DIR}/config_yaml/viewer_app_config.yaml.partial")
configure_file("cmake/viewer_app_config.yaml.in" "${_ADD_CONFIG_YAML_PARTIAL_FILE}")
oe_target_add_config_yaml(${PROJECT_NAME} "${_ADD_CONFIG_YAML_PARTIAL_FILE}")

oe_target_build_config_yaml(${PROJECT_NAME} SOURCE_TARGETS ${PROJECT_NAME} Oe::App Oe::Core Oe::Scripting)

#####
# Post-build: copy python runtime
#####
add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy "${OeScripting_PYTHON_RUNTIME_DLL}" "${PROJECT_BINARY_DIR}"
        )
