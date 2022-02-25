project(OeScripting VERSION 1.0
        DESCRIPTION "Orangine Scripting Library"
        LANGUAGES CXX)

######################
# Library Definition
add_library(${PROJECT_NAME} STATIC
        src/OeScripting_bindings.cpp
        src/Entity_scripting_manager.cpp        
        src/Entity_scripting_manager.h
        src/Script_runtime_data.h
        src/Statics.cpp
        src/OeScripting_embedded_module.cpp
        src/OeScripting.cpp
        # TODO: Probably don't need this any more?
        include/OeScripting/OeScripting_Export.h src/OeScripting_embedded_module.h)

file(GLOB OeScripting_HEADERS "include/OeScripting/*")
list(APPEND OeScripting_HEADERS "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h")

add_library(Oe::Scripting ALIAS OeScripting)

include (GenerateExportHeader)
GENERATE_EXPORT_HEADER(OeScripting
        BASE_NAME OeScripting
        EXPORT_MACRO_NAME OeScripting_EXPORT
        EXPORT_FILE_NAME include/OeScripting/OeScripting_Export.h
        STATIC_DEFINE OeScripting_BUILT_AS_STATIC
        )

#####
# Dependencies
#####
target_link_libraries(${PROJECT_NAME}
        PUBLIC OeCompilerFlags pybind11::embed pybind11::python_link_helper
        PRIVATE Oe::Core)

#####
# Orangine module configuration
#####

# configure /cmake folder and src/{PROJECT_NAME}Config.h.in file
oe_target_configure_cmake_dir(${PROJECT_NAME})

# configure /include folder (public header files) and configure .cmake files
oe_target_configure_include_dir(${PROJECT_NAME})

# configure /scripts folder
oe_target_add_scripts_dir(${PROJECT_NAME} HAS_SCRIPTS MODULES oe_bindings)

# Precompiled Headers: disabled as python must be included prior to any STL files. Don't want to consume the PUBLIC
# PCH of OeCore.
set_target_properties(${PROJECT_NAME} PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)

# Make sure to include PDB files when installing in debug mode
oe_target_install_pdb(${PROJECT_NAME})

#####
# Configuration file blending
#####
set(OeScripting_PYENV_DIR "$ENV{VIRTUAL_ENV}")
set(OeScripting_PYENV_DIR $ENV{VIRTUAL_ENV} PARENT_SCOPE)
set(_ADD_CONFIG_YAML_PARTIAL_FILE "${PROJECT_BINARY_DIR}/config_yaml/oe_scripting.yaml.partial")
configure_file("cmake/oe_scripting.yaml.in" "${_ADD_CONFIG_YAML_PARTIAL_FILE}")
oe_target_add_config_yaml(${PROJECT_NAME} "${_ADD_CONFIG_YAML_PARTIAL_FILE}")

#####
# Custom Install Steps
#####

# Install Python binaries
if (${OE_PYTHON_DEBUG})
        set(OeScripting_PYTHON_RUNTIME_DLL "${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}_d.dll")
else ()
        set(OeScripting_PYTHON_RUNTIME_DLL "${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.dll")
endif()
install(FILES "${OeScripting_PYTHON_RUNTIME_DLL}" DESTINATION bin)
set(OeScripting_PYTHON_RUNTIME_DLL ${OeScripting_PYTHON_RUNTIME_DLL} PARENT_SCOPE)
