# Installs PDB files in debug builds
function(oe_target_install_pdb _target)
        set(TGT_NAME $<TARGET_PROPERTY:${_target},NAME>)
        if(CMAKE_BUILD_TYPE MATCHES Debug)
                get_target_property(TGT_TYPE ${_target} TYPE)
                if(TGT_TYPE MATCHES STATIC_LIBRARY)
                        set(OE_PDB_NAME "${_target}${CMAKE_DEBUG_POSTFIX}")
                        set_target_properties(${_target} PROPERTIES
                                COMPILE_PDB_NAME ${OE_PDB_NAME})
                        install(FILES $<TARGET_FILE_DIR:${TGT_NAME}>/${OE_PDB_NAME}.pdb DESTINATION lib)
                else()
                        install(FILES $<TARGET_FILE_DIR:${TGT_NAME}>/${_target}.pdb DESTINATION bin)
                endif()
        endif()
endfunction()

function(oe_target_configure_cmake_dir _target)
        get_target_property(TGT_NAME ${_target} NAME)
        get_target_property(TGT_BINARY_DIR ${_target} BINARY_DIR)
        get_target_property(TGT_SOURCE_DIR ${_target} SOURCE_DIR)

        ####
        # cmake config files
        configure_file (
                "${TGT_SOURCE_DIR}/src/${TGT_NAME}Config.h.in"
                "${TGT_BINARY_DIR}/include/${TGT_NAME}Config.h"
        )
        configure_file(cmake/${TGT_NAME}Config.cmake.in ${TGT_NAME}Config.cmake @ONLY)
        configure_file(cmake/${TGT_NAME}ConfigVersion.cmake.in ${TGT_NAME}ConfigVersion.cmake @ONLY)
        install(
                FILES "${TGT_BINARY_DIR}/${TGT_NAME}Config.cmake"
                "${TGT_BINARY_DIR}/${TGT_NAME}ConfigVersion.cmake"
                DESTINATION lib/cmake/${TGT_NAME}
        )
endfunction()

function(oe_target_generate_enum _target)
        cmake_parse_arguments(_GENERATE_ENUM "" "SOURCE;TARGET_SUFFIX" "BYPRODUCTS" ${ARGN})
        get_target_property(TGT_NAME ${_target} NAME)

        if(NOT _GENERATE_ENUM_SOURCE)
                message(FATAL_ERROR "oe_target_generate_enum: SOURCE is not set")
        endif()
        if(NOT _GENERATE_ENUM_TARGET_SUFFIX)
                set(_GENERATE_ENUM_TARGET_SUFFIX "_codegen")
        endif()

        if(NOT Python3_EXECUTABLE)
                message(FATAL_ERROR "oe_target_generate_enum: Python3_EXECUTABLE is not set")
        endif()

        # enum codegen
        add_custom_target(
                ${TGT_NAME}${_GENERATE_ENUM_TARGET_SUFFIX} ALL
                COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../scripts/codegen-enum.py" --json "${_GENERATE_ENUM_SOURCE}"
                BYPRODUCTS ${_GENERATE_ENUM_BYPRODUCTS}
                COMMENT "Generating ${TGT_NAME} enums: ${_GENERATE_ENUM_SOURCE}")
        add_dependencies(${TGT_NAME} ${TGT_NAME}${_GENERATE_ENUM_TARGET_SUFFIX})
endfunction()

# Add standard OE static library headers & .cmake files
function(oe_target_configure_include_dir _target)
        get_target_property(TGT_NAME ${_target} NAME)
        get_target_property(TGT_BINARY_DIR ${_target} BINARY_DIR)
        get_target_property(TGT_SOURCE_DIR ${_target} SOURCE_DIR)

        ####
        # public header files (includes)
        file(GLOB ${TGT_NAME}_HEADERS "include/${TGT_NAME}/*")
        list(APPEND ${TGT_NAME}_HEADERS "${TGT_BINARY_DIR}/include/${TGT_NAME}Config.h")

        target_include_directories(${TGT_NAME}
                PUBLIC
                $<BUILD_INTERFACE:${TGT_BINARY_DIR}/include>
                $<BUILD_INTERFACE:${TGT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
                )

        install(
                FILES ${${TGT_NAME}_HEADERS}
                DESTINATION include/${TGT_NAME}
        )
endfunction()

function(oe_target_add_data_dir _target)
        cmake_parse_arguments(_ADD_DATA "NO_INSTALL" "" "" ${ARGN})

        get_target_property(TGT_NAME ${_target} NAME)
        get_target_property(TGT_SOURCE_DIR ${_target} SOURCE_DIR)
        get_target_property(TGT_BINARY_DIR ${_target} BINARY_DIR)

        # This targets data and Assets
        set(DATA_SOURCE_DIRS "${TGT_NAME}=${TGT_SOURCE_DIR}/data")
        set_target_properties(${TGT_NAME} PROPERTIES OE_DATA_SOURCE_DIRS_NO_DEPENDENCIES "${DATA_SOURCE_DIRS}")

        get_property(_TGT_LINK_LIBRARIES TARGET ${TGT_NAME} PROPERTY LINK_LIBRARIES SET)
        if (_TGT_LINK_LIBRARIES)
                get_target_property(_TGT_LINK_LIBRARIES ${TGT_NAME} LINK_LIBRARIES)
        else()
                set(_TGT_LINK_LIBRARIES "")
        endif()

        # Dependencies data and Assets
        if (_TGT_LINK_LIBRARIES)
                foreach(_dependency ${_TGT_LINK_LIBRARIES})
                        if (NOT TARGET ${_dependency})
                                continue()
                        endif()
                        get_property(_TGT_DEP_DATA_SOURCE_DIR TARGET ${_dependency} PROPERTY OE_SCRIPT_SOURCE_DIRS SET)
                        if (_TGT_DEP_DATA_SOURCE_DIR)
                                list(APPEND DATA_SOURCE_DIRS "${_TGT_DEP_DATA_SOURCE_DIR}")
                        endif()
                endforeach()
        endif()

        # This targets data and Assets
        set(${TGT_NAME}_DATA_SOURCE_DIRS "${DATA_SOURCE_DIRS}" PARENT_SCOPE)
        set_target_properties(${TGT_NAME} PROPERTIES OE_DATA_SOURCE_DIRS "${DATA_SOURCE_DIRS}")

        if (NOT _ADD_DATA_NO_INSTALL)
                install(DIRECTORY data DESTINATION bin)
        endif()

        # Configuration YAML files
        set(_ADD_DATA_YAML_PARTIAL_FILE "${TGT_BINARY_DIR}/config_yaml/data_paths.yaml.partial")
        configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/yaml_templates/data_paths.yaml.in" "${_ADD_DATA_YAML_PARTIAL_FILE}")
        oe_target_add_config_yaml(${_target} "${_ADD_DATA_YAML_PARTIAL_FILE}")
endfunction()

########
# BEGIN YAML File creation
function(oe_target_add_config_yaml _target _filename)
        set_property(TARGET ${_target} APPEND PROPERTY OE_BUILD_YAML_PARTIALS "${_filename}")
endfunction()

define_property(TARGET PROPERTY OE_BUILD_YAML_PARTIALS
        BRIEF_DOCS "A list of yaml files to merge"
        FULL_DOCS "Each file in the list will be read by oe_target_build_config_yaml and merged into a single output yaml file")

# Creates a target that merges all YAML files the the  registered via `oe_target_add_config_yaml`
function(oe_target_build_config_yaml _target)
        cmake_parse_arguments(_BUILD_YAML "" "OUTPUT_NAME" "SOURCE_TARGETS" ${ARGN})
        get_target_property(TGT_NAME ${_target} NAME)
        get_target_property(TGT_BINARY_DIR ${_target} BINARY_DIR)
        get_target_property(TGT_SOURCE_DIR ${_target} SOURCE_DIR)

        if(NOT _BUILD_YAML_OUTPUT_NAME)
                set(_BUILD_YAML_OUTPUT_NAME "config.yaml")
        endif()

        set(_BUILD_YAML_MERGE_SOURCES "")

        foreach(_dependency ${_BUILD_YAML_SOURCE_TARGETS})
                get_target_property(DEP_TGT_BUILD_YAML_PARTIALS ${_dependency} OE_BUILD_YAML_PARTIALS)
                if ("${DEP_TGT_BUILD_YAML_PARTIALS}" MATCHES "DEP_TGT_BUILD_YAML_PARTIALS-NOTFOUND")
                        set(DEP_TGT_BUILD_YAML_PARTIALS "")
                endif()
                list(APPEND _BUILD_YAML_MERGE_SOURCES "${DEP_TGT_BUILD_YAML_PARTIALS}")
        endforeach()

        list(LENGTH _BUILD_YAML_MERGE_SOURCES _BUILD_YAML_MERGE_SOURCES_LEN)
        if (_BUILD_YAML_MERGE_SOURCES_LEN EQUAL 0)
                message(FATAL_ERROR "oe_target_build_config_yaml: No SOURCE_TARGETS have the OE_BUILD_YAML_PARTIALS property defined. Use oe_target_add_config_yaml() to register yaml files.")
        endif()

        if(NOT Python3_EXECUTABLE)
                message(FATAL_ERROR "oe_target_generate_enum: Python3_EXECUTABLE is not set")
        endif()

        add_custom_command(
                OUTPUT ${TGT_NAME}_build_yaml_command
                COMMENT "${_target}: Merging YAML config"
                COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../scripts/merge_yaml.py" "${_BUILD_YAML_MERGE_SOURCES}" > "${TGT_BINARY_DIR}/${_BUILD_YAML_OUTPUT_NAME}"
                )
        add_custom_target(${TGT_NAME}_build_yaml DEPENDS ${TGT_NAME}_build_yaml_command)
        add_dependencies(${TGT_NAME} ${TGT_NAME}_build_yaml)
endfunction()

#####
# END YAML File creation