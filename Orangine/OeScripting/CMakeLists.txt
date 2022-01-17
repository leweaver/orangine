project(OeScripting VERSION 1.0
        DESCRIPTION "Orangine Scripting Library"
        LANGUAGES CXX)


######################
# Load Python Dependencies

# Use python debug binaries (with PyDEBUG defined)?
# Note - all pip modules must also be built with PyDEBUG.
SET(OeScripting_PYTHON_DEBUG "OFF" CACHE BOOL "Use python debug binaries (built with PyDEBUG defined)")
SET(OeScripting_PYTHON_DEBUG ${OeScripting_PYTHON_DEBUG} PARENT_SCOPE)

include (OePython)

MESSAGE(STATUS "Verifying pip Packages: $ENV{VIRTUAL_ENV}")
execute_process(
        COMMAND "$ENV{VIRTUAL_ENV}/Scripts/pip3.exe"
        install -r "${PROJECT_SOURCE_DIR}/pyenv-requirements.txt"
        RESULT_VARIABLE _retval)
IF (NOT "${_retval}" MATCHES "0")
    MESSAGE(FATAL_ERROR "Failed to install virtualenv requirements")
endif()

######################
# Library Definition
add_library(${PROJECT_NAME} STATIC
        src/pch.h
        src/pch.cpp
        src/OeScripting_bindings.cpp
        src/Entity_scripting_manager.cpp        
        src/Entity_scripting_manager.h
        src/Script_runtime_data.h
        src/Statics.cpp
        src/OeScripting_embedded_module.cpp
        src/OeScripting.cpp
        # TODO: Probably don't need this any more?
        include/OeScripting/OeScripting_Export.h)

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

target_link_libraries(${PROJECT_NAME}
        PUBLIC OeCompilerFlags
        PRIVATE Oe::Core)

# Python Scripts Dir
oe_add_scripts_dir(OeScripting oe_bindings
        LINK_LIBRARIES Oe::Core)

# Precompiled Headers
include(PrecompiledHeader)
add_precompiled_header(${PROJECT_NAME} pch.h
        SOURCE_CXX src/pch.cpp
        IGNORE_FILES src/OeScripting_bindings.cpp)

# Public headers
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )

target_link_libraries(${PROJECT_NAME} PUBLIC pybind11::embed pybind11::python_link_helper)

IF (NOT "${_retval}" MATCHES "0")
    MESSAGE(FATAL_ERROR "Failed to create pyi")
endif()
#python "stubgen --search-path, C:\\repos\\orangine\\Orangine\\build\\VS_2019_AMD64_Debug, -m, OeScripting.oe"

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
install(FILES ${PROJECT_BINARY_DIR}/oe.pyi
        DESTINATION bin/data/${PROJECT_NAME}/lib
        )

message(NOTICE "Python_LIBRARY_DIRS=${Python_LIBRARY_DIRS}")
message(NOTICE "Python_INCLUDE_DIRS=${Python_INCLUDE_DIRS}")
message(NOTICE "Python_RUNTIME_LIBRARY_DIRS=${Python_RUNTIME_LIBRARY_DIRS}")

# public header files
install(
        FILES ${OeScripting_HEADERS}
        DESTINATION include/${PROJECT_NAME}
)

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