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
        src/Engine_bindings.cpp
        src/Engine_bindings.h
        src/Entity_scripting_manager.cpp        
        src/Entity_scripting_manager.h
        src/Script_runtime_data.h
        src/Statics.cpp
        src/Engine_internal_module.cpp src/Engine_internal_module.h src/OeScripting.cpp)

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

# Python Env
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# It is critical that the version of python we compile against matches the scripts loaded into the virtualenv
if (NOT "${Python3_VERSION}" VERSION_GREATER_EQUAL "3.7.6")
    message(FATAL_ERROR "Found python ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}, but must be compiled against minimum of 3.7.6")
endif()

# Python search paths
if ("${Orangine_ARCHITECTURE}" MATCHES "x86")
    set(ENV{VIRTUAL_ENV} "${PROJECT_BINARY_DIR}/pyenv")
elseif ("${Orangine_ARCHITECTURE}" MATCHES "x64")
    set(ENV{VIRTUAL_ENV} "${PROJECT_BINARY_DIR}/pyenv")
else ()
    MESSAGE(ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif ()
if (NOT EXISTS "$ENV{VIRTUAL_ENV}")
    MESSAGE("Creating python environment: $ENV{VIRTUAL_ENV}")
    execute_process(COMMAND "${Python3_EXECUTABLE}" -m venv "$ENV{VIRTUAL_ENV}"
            RESULT_VARIABLE _retval)
    IF (NOT "${_retval}" MATCHES "0")
        MESSAGE(FATAL_ERROR "Failed to create venv")
    endif()
endif()

set(Python_FIND_VIRTUALENV ONLY)
set(Python3_FIND_VIRTUALENV ONLY)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

MESSAGE(STATUS "Verifying pip Packages: $ENV{VIRTUAL_ENV}")
execute_process(
        COMMAND "$ENV{VIRTUAL_ENV}/Scripts/pip3.exe"
        install -r "${PROJECT_SOURCE_DIR}/pyenv-requirements.txt"
        RESULT_VARIABLE _retval)
IF (NOT "${_retval}" MATCHES "0")
    MESSAGE(FATAL_ERROR "Failed to install virtualenv requirements")
endif()

# PyBind
find_package(pybind11 REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC pybind11::embed pybind11::python_link_helper)


# --------------------------------
# Install
include(OeUtils)
install_target_pdb(OeScripting)

# Data and Assets
set(OeScripting_PYENV_DIR "$ENV{VIRTUAL_ENV}$<$<BOOL:${OeScripting_PYTHON_DEBUG}>:_d>")
set(OeScripting_PYENV_DIR $ENV{VIRTUAL_ENV} PARENT_SCOPE)
set(${PROJECT_NAME}_DATA_SOURCE_DIRS "${PROJECT_NAME}=${PROJECT_SOURCE_DIR}/data;pyenv=${OeScripting_PYENV_DIR}")
set(${PROJECT_NAME}_DATA_SOURCE_DIRS "${${PROJECT_NAME}_DATA_SOURCE_DIRS}" PARENT_SCOPE)
install(DIRECTORY data
        DESTINATION bin
        )

message(NOTICE "Python_LIBRARY_DIRS=${Python_LIBRARY_DIRS}")
message(NOTICE "Python_INCLUDE_DIRS=${Python_INCLUDE_DIRS}")
message(NOTICE "Python_RUNTIME_LIBRARY_DIRS=${Python_RUNTIME_LIBRARY_DIRS}")

# Install Python binaries
if (${OeScripting_PYTHON_DEBUG})
        set(OeScripting_PYTHON_RUNTIME_DLL "${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}_d.dll")
else ()
        set(OeScripting_PYTHON_RUNTIME_DLL "${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.dll")
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