# Python Env
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# It is critical that the version of python we compile against matches the scripts loaded into the virtualenv
if (NOT "${Python3_VERSION}" VERSION_GREATER_EQUAL "3.7.6")
    message(FATAL_ERROR "Found python ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}, but must be compiled against minimum of 3.7.6")
endif()

# Python search paths
cmake_path(SET PROJECT_PARENT_BINARY_DIR NORMALIZE "${PROJECT_BINARY_DIR}/..")
if ("${Orangine_ARCHITECTURE}" MATCHES "x86")
    set(ENV{VIRTUAL_ENV} "${PROJECT_PARENT_BINARY_DIR}/pyenv")
elseif ("${Orangine_ARCHITECTURE}" MATCHES "x64")
    set(ENV{VIRTUAL_ENV} "${PROJECT_PARENT_BINARY_DIR}/pyenv")
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

# PyBind
find_package(pybind11 REQUIRED)

# Adds python scripting
function(oe_add_scripts_dir _target _moduleName)
    cmake_parse_arguments(_TGT "" "" "LINK_LIBRARIES" ${ARGN})

    get_target_property(TGT_NAME ${_target} NAME)
    get_target_property(TGT_BINARY_DIR ${_target} BINARY_DIR)
    get_target_property(TGT_SOURCE_DIR ${_target} SOURCE_DIR)

    ##########################
    # .pyi generation for native bindings
    # need to recompile the module as a standalone, so that it can be imported.
    pybind11_add_module(${_moduleName} MODULE src/${TGT_NAME}_bindings.cpp src/${TGT_NAME}_standalone_module.cpp)
    target_include_directories(${_moduleName}
            PUBLIC
            $<BUILD_INTERFACE:${TGT_BINARY_DIR}/include>
            $<BUILD_INTERFACE:${TGT_SOURCE_DIR}/include>
            )
    target_link_libraries(${_moduleName} PRIVATE ${_TGT_LINK_LIBRARIES})
    cmake_path(SET PROJECT_PARENT_BINARY_DIR NORMALIZE "${TGT_BINARY_DIR}/..")
    add_custom_command(OUTPUT ${TGT_NAME}_pyi_create
            WORKING_DIRECTORY ${PROJECT_PARENT_BINARY_DIR}
            VERBATIM
            COMMAND $ENV{VIRTUAL_ENV}/Scripts/Activate.bat
            COMMAND  stubgen -q -m ${TGT_NAME}.${_moduleName} -o ${PROJECT_PARENT_BINARY_DIR}
            # copy command doesn't work unless windows backslashes are used.
            #COMMAND echo F|xcopy ${TGT_NAME}\\${_moduleName}.pyi ..\\..\\${TGT_NAME}\\bindings.pyi /y /q
            COMMAND python -c "import shutil; shutil.move('${TGT_NAME}/${_moduleName}.pyi', '${TGT_SOURCE_DIR}/bindings.pyi')"
            COMMENT "Generating ${TGT_NAME}/bindings.pyi")

    add_custom_target(${TGT_NAME}_pyi DEPENDS ${TGT_NAME}_pyi_create)
    add_dependencies(${TGT_NAME}_pyi ${_moduleName})
    add_dependencies(${TGT_NAME} ${TGT_NAME}_pyi)
endfunction()