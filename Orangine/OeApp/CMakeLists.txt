project(OeApp VERSION 1.0
        DESCRIPTION "Orangine App Library"
        LANGUAGES CXX)

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
        src/OeApp_embedded_module.cpp src/Yaml_config_reader.cpp)
add_library(Oe::App ALIAS OeApp)

#####
# Dependencies
#####
target_link_libraries(${PROJECT_NAME}
        PUBLIC WindowsApp.lib OeCompilerFlags Oe::Core Oe::Scripting)

# yaml
find_package(yaml-cpp REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC yaml-cpp::yaml-cpp)

#####
# Testing
#####
if(OE_BUILD_TESTING)
    add_subdirectory(tests)
endif()

#####
# Orangine module configuration
#####

# configure /cmake folder and src/{PROJECT_NAME}Config.h.in file
oe_target_configure_cmake_dir(${PROJECT_NAME})

# configure /include folder (public header files) and configure .cmake files
oe_target_configure_include_dir(${PROJECT_NAME})

# configure /data folder
oe_target_add_data_dir(${PROJECT_NAME})

# configure /scripts folder
oe_target_add_scripts_dir(${PROJECT_NAME} oe_app_bindings
        LINK_LIBRARIES Oe::Core Oe::Scripting)

# Precompiled Headers
include(PrecompiledHeader)
add_precompiled_header(${PROJECT_NAME} pch.h 
        SOURCE_CXX src/pch.cpp
        IGNORE_FILES src/${PROJECT_NAME}_bindings.cpp
        )

# Make sure to include PDB files when installing in debug mode
oe_target_install_pdb(${PROJECT_NAME})

