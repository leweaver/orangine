function(oe_create_python_env)
    # Python Env
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    # It is critical that the version of python we compile against matches the scripts loaded into the virtualenv
    if (NOT "${Python3_VERSION}" VERSION_GREATER_EQUAL "3.7.6")
        message(FATAL_ERROR "Found python ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}, but must be compiled against minimum of 3.7.6")
    endif()

    MESSAGE("Creating python environment: $ENV{VIRTUAL_ENV}")
    execute_process(COMMAND "${Python3_EXECUTABLE}" -m venv "$ENV{VIRTUAL_ENV}"
            RESULT_VARIABLE _retval)
    IF (NOT "${_retval}" MATCHES "0")
        MESSAGE(FATAL_ERROR "Failed to create venv")
    endif()
endfunction()

# Use python debug binaries (with PyDEBUG defined)?
# Note - all pip modules must also be built with PyDEBUG.
SET(OE_PYTHON_DEBUG "OFF" CACHE BOOL "Use python debug binaries (built with PyDEBUG defined)")

# Python search paths
set(ENV{VIRTUAL_ENV} "${CMAKE_BINARY_DIR}/pyenv")
if (NOT EXISTS "$ENV{VIRTUAL_ENV}")
    oe_create_python_env()
endif()

set(Python_FIND_VIRTUALENV ONLY)
set(Python3_FIND_VIRTUALENV ONLY)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# PyBind
find_package(pybind11 REQUIRED)

# Pip packages
MESSAGE(STATUS "Verifying pip Packages: $ENV{VIRTUAL_ENV}")
execute_process(
        COMMAND "$ENV{VIRTUAL_ENV}/Scripts/pip3.exe"
        install --disable-pip-version-check -r "${PROJECT_SOURCE_DIR}/pyenv-requirements.txt"
        RESULT_VARIABLE _retval)
IF (NOT "${_retval}" MATCHES "0")
    MESSAGE(FATAL_ERROR "Failed to install virtualenv requirements")
endif()

# Adds python scripting
function(oe_target_add_scripts_dir _target)
    cmake_parse_arguments(_TGT "HAS_SCRIPTS;NO_INSTALL" "" "MODULES;DEPENDENCIES" ${ARGN})

    get_target_property(TGT_NAME ${_target} NAME)
    get_target_property(TGT_BINARY_DIR ${_target} BINARY_DIR)
    get_target_property(TGT_SOURCE_DIR ${_target} SOURCE_DIR)

    get_property(_TGT_LINK_LIBRARIES TARGET ${TGT_NAME} PROPERTY LINK_LIBRARIES SET)
    if (_TGT_LINK_LIBRARIES)
        get_target_property(_TGT_LINK_LIBRARIES ${TGT_NAME} LINK_LIBRARIES)
    else()
        set(_TGT_LINK_LIBRARIES "")
    endif()

    if (_TGT_MODULES)
        foreach (_moduleName ${_TGT_MODULES})
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
            add_custom_command(OUTPUT ${TGT_NAME}_${_moduleName}_pyi_create
                    WORKING_DIRECTORY ${PROJECT_PARENT_BINARY_DIR}
                    VERBATIM
                    COMMAND $ENV{VIRTUAL_ENV}/Scripts/Activate.bat
                    COMMAND  stubgen -m ${TGT_NAME}.${_moduleName} -o $ENV{VIRTUAL_ENV}/Lib/site-packages
                    # copy command doesn't work unless windows backslashes are used.
                    #COMMAND echo F|xcopy ${TGT_NAME}\\${_moduleName}.pyi ..\\..\\${TGT_NAME}\\bindings.pyi /y /q
                    #COMMAND python -c "import shutil; shutil.move('${TGT_NAME}/${_moduleName}.pyi', '${TGT_SOURCE_DIR}/${_moduleName}.pyi')"
                    COMMENT "Generating ${TGT_NAME}/${_moduleName}.pyi")
            IF (NOT "${_retval}" MATCHES "0")
                MESSAGE(FATAL_ERROR "Failed to create pyi")
            endif()

            add_custom_target(${TGT_NAME}_${_moduleName}_pyi DEPENDS ${TGT_NAME}_${_moduleName}_pyi_create)
            add_dependencies(${TGT_NAME}_${_moduleName}_pyi ${_moduleName})
            add_dependencies(${TGT_NAME} ${TGT_NAME}_${_moduleName}_pyi)

            if (NOT _TGT_NO_INSTALL)
                install(FILES ${TGT_BINARY_DIR}/${_moduleName}.pyi
                        DESTINATION bin/data/${TGT_NAME}/lib
                        )
            endif()
        endforeach(_moduleName)
    endif()
    if (_TGT_HAS_SCRIPTS)
        # This targets scripts and Assets
        set(SCRIPT_SOURCE_DIRS "${TGT_NAME}=${TGT_SOURCE_DIR}/scripts")
        set_target_properties(${TGT_NAME} PROPERTIES OE_SCRIPT_SOURCE_DIRS_NO_DEPENDENCIES "${SCRIPT_SOURCE_DIRS}")

        # Dependencies scripts and Assets
        if (_TGT_LINK_LIBRARIES)
            foreach(_dependency ${_TGT_LINK_LIBRARIES})
                if (NOT TARGET ${_dependency})
                        continue()
                endif()
                get_property(_TGT_DEP_DATA_SOURCE_DIR TARGET ${_dependency} PROPERTY OE_SCRIPT_SOURCE_DIRS SET)
                if (_TGT_DEP_DATA_SOURCE_DIR)
                    get_target_property(_TGT_DEP_DATA_SOURCE_DIR ${_dependency} OE_SCRIPT_SOURCE_DIRS)
                    list(APPEND SCRIPT_SOURCE_DIRS "${_TGT_DEP_DATA_SOURCE_DIR}")
                endif()
            endforeach()
        endif()

        # This targets scripts and Assets
        set(${TGT_NAME}_SCRIPT_SOURCE_DIRS "${SCRIPT_SOURCE_DIRS}" PARENT_SCOPE)
        set_target_properties(${TGT_NAME} PROPERTIES OE_SCRIPT_SOURCE_DIRS "${SCRIPT_SOURCE_DIRS}")

        # Create pyi files
        string(TOLOWER ${TGT_NAME} TGT_NAME_LOWER)
        add_custom_command(OUTPUT ${TGT_NAME}_scripts_pyi_create
                WORKING_DIRECTORY ${TGT_SOURCE_DIR}/scripts
                VERBATIM
                COMMAND $ENV{VIRTUAL_ENV}/Scripts/Activate.bat
                COMMAND stubgen -p ${TGT_NAME_LOWER} -o $ENV{VIRTUAL_ENV}/Lib/site-packages
                COMMENT "Generating ${TGT_NAME_LOWER} pyi files")
        IF (NOT "${_retval}" MATCHES "0")
            MESSAGE(FATAL_ERROR "Failed to create pyi")
        endif()

        add_custom_target(${TGT_NAME}_scripts_pyi DEPENDS ${TGT_NAME}_scripts_pyi_create)
        add_dependencies(${TGT_NAME} ${TGT_NAME}_scripts_pyi)

        if (NOT _TGT_NO_INSTALL)
            install(DIRECTORY scripts DESTINATION bin)
        endif()

        # Configuration YAML files
        set(_TGT_YAML_PARTIAL_FILE "${TGT_BINARY_DIR}/config_yaml/script_paths.yaml.partial")
        configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/yaml_templates/script_paths.yaml.in" "${_TGT_YAML_PARTIAL_FILE}")
        oe_target_add_config_yaml(${_target} "${_TGT_YAML_PARTIAL_FILE}")
    endif()
endfunction(oe_target_add_scripts_dir)