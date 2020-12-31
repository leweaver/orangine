project(OeScripting VERSION 1.0
        DESCRIPTION "Orangine Scripting Library"
        LANGUAGES CXX)

# Use python debug binaries (with PyDEBUG defined)?
# Note - all pip modules must also be built with PyDEBUG.
SET(OeScripting_PYTHON_DEBUG "OFF" CACHE BOOL "Use python debug binaries (built with PyDEBUG defined)")
SET(OeScripting_PYTHON_DEBUG ${OeScripting_PYTHON_DEBUG} PARENT_SCOPE)

# Library Definition
add_library(${PROJECT_NAME} STATIC
        src/pch.h
        src/pch.cpp
        src/Binding_helpers.cpp
        src/Engine_bindings.cpp
        src/Engine_bindings.h
        src/Entity_scripting_manager.cpp        
        src/Entity_scripting_manager.h
        src/Script_runtime_data.h
        )

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

# PyBind
# Override the PYTHON_EXECUTABLE variable with the results of FindPython3, so that pybind 
# uses the 32-bit interpreter in x86 builds.
# Get the name of the python runtime DLL
find_package(Python COMPONENTS Interpreter Development REQUIRED)
# set(PYTHON_EXECUTABLE "${Python_EXECUTABLE}")
# set(PYBIND11_CPP_STANDARD /std:c++17)
find_package(pybind11 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC pybind11::embed pybind11::python_link_helper)

# --------------------------------
# Install
install(TARGETS ${PROJECT_NAME}
        EXPORT OeScriptingTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
        )
		
include(OeUtils)
install_target_pdb(OeScripting)

install(
        EXPORT OeScriptingTargets
        FILE OeScriptingTargets.cmake
        NAMESPACE Oe::
        DESTINATION lib/cmake/${PROJECT_NAME}
)

# Data and Assets
set(OeScripting_PYENV_DIR "${OE_THIRDPARTY_PATH}/pyenv_37_${Orangine_ARCHITECTURE}$<$<BOOL:${OeScripting_PYTHON_DEBUG}>:_d>")
set(OeScripting_PYENV_DIR ${OeScripting_PYENV_DIR} PARENT_SCOPE)
set(${PROJECT_NAME}_DATA_SOURCE_DIRS "${PROJECT_NAME}=${PROJECT_SOURCE_DIR}/data;pyenv=${OeScripting_PYENV_DIR}")
set(${PROJECT_NAME}_DATA_SOURCE_DIRS "${${PROJECT_NAME}_DATA_SOURCE_DIRS}" PARENT_SCOPE)
install(DIRECTORY data
        DESTINATION bin
        )

# Install Python binaries
get_filename_component(OeScripting_PYTHON_RUNTIME_DLL "${Python_EXECUTABLE}" DIRECTORY)
if (${OeScripting_PYTHON_DEBUG})
        set(OeScripting_PYTHON_RUNTIME_DLL "${OeScripting_PYTHON_RUNTIME_DLL}/python${Python_VERSION_MAJOR}${Python_VERSION_MINOR}_d.dll")
else ()
        set(OeScripting_PYTHON_RUNTIME_DLL "${OeScripting_PYTHON_RUNTIME_DLL}/python${Python_VERSION_MAJOR}${Python_VERSION_MINOR}.dll")
endif()
install(FILES "${OeScripting_PYTHON_RUNTIME_DLL}" DESTINATION bin)
set(OeScripting_PYTHON_RUNTIME_DLL ${OeScripting_PYTHON_RUNTIME_DLL} PARENT_SCOPE)

# Config.cmake files
configure_file (
        "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}Config.h.in"
        "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h"
)

configure_file(OeScriptingConfig.cmake.in OeScriptingConfig.cmake @ONLY)
configure_file(OeScriptingConfigVersion.cmake.in OeScriptingConfigVersion.cmake @ONLY)
install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/OeScriptingConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/OeScriptingConfigVersion.cmake"
        DESTINATION lib/cmake/OeScripting
)