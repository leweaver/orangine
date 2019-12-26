project(ViewerApp VERSION 1.0
        DESCRIPTION "Orangine Sample Application"
        LANGUAGES CXX)

# --------------------------------
# Configure File
set(ViewerApp_THIRDPARTY_PATH ${CMAKE_SOURCE_DIR}/../thirdparty)
configure_file (
        "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}Config.h.in"
        "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h"
)

# --------------------------------
# Library Definition
add_executable(${PROJECT_NAME} WIN32
        ${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h
        src/pch.h
        src/pch.cpp
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
include(PrecompiledHeader)
add_precompiled_header(${PROJECT_NAME} pch.h
        SOURCE_CXX src/pch.cpp)

# --------------------------------
# Dependencies

# Use this for embedding in the built application, but also for some build scripts in CMake
find_package(Python3 COMPONENTS Development Interpreter)

target_link_libraries(${PROJECT_NAME} PUBLIC OeCompilerFlags)
target_link_libraries(${PROJECT_NAME} PRIVATE Oe::Core Oe::Scripting Oe::App)

# Manually copy the python DLL for builds, to keep the Py_SetPythonHome path < 260 characters.
set(Python3_DLL_NAME "python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}$<$<CONFIG:debug>:_d>$<$<CONFIG:release>:>.dll")
add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND "${CMAKE_COMMAND}" -E copy "${Python3_RUNTIME_LIBRARY_DIRS}/${Python3_DLL_NAME}" "${PROJECT_BINARY_DIR}"
                   )

# --------------------------------
# Configuration file blending
set(ViewerApp_DATA_DIRECTORIES "${PROJECT_NAME}=${PROJECT_SOURCE_DIR}/data")
list(APPEND ViewerApp_DATA_DIRECTORIES "OeCore=${OeCore_DATA_SOURCE_DIR}")
list(APPEND ViewerApp_DATA_DIRECTORIES "OeApp=${OeApp_DATA_SOURCE_DIR}")
list(APPEND ViewerApp_DATA_DIRECTORIES "OeScripting=${OeScripting_DATA_SOURCE_DIR}")
set(ViewerApp_SOURCE_CONFIG_FILE "${PROJECT_SOURCE_DIR}/src/config.default.json")

# Write source data paths, for cases where the app is run from the build output folder
set(ViewerApp_DATA_PATHS_CONFIG_FILE "${PROJECT_BINARY_DIR}/config.data_paths.json")
set(ViewerApp_BUILD_CONFIG_FILE "${PROJECT_BINARY_DIR}/config.json")
add_custom_command(OUTPUT "${ViewerApp_DATA_PATHS_CONFIG_FILE}"
		COMMENT "Creating data_paths configuration file"
		COMMAND "${Python3_EXECUTABLE}" "${Orangine_SOURCE_DIR}/cmake/scripts/make_data_paths_config.py" "${ViewerApp_DATA_DIRECTORIES}" > "${ViewerApp_DATA_PATHS_CONFIG_FILE}"
		)
add_custom_command(OUTPUT "${ViewerApp_BUILD_CONFIG_FILE}"
		COMMENT "Creating build configuration file"
		COMMAND "${Python3_EXECUTABLE}" "${Orangine_SOURCE_DIR}/cmake/scripts/merge_config.py" "${ViewerApp_SOURCE_CONFIG_FILE}" "${ViewerApp_DATA_PATHS_CONFIG_FILE}" > "${ViewerApp_BUILD_CONFIG_FILE}"
		DEPENDS "${ViewerApp_SOURCE_CONFIG_FILE}" "${ViewerApp_DATA_PATHS_CONFIG_FILE}"
		)
add_custom_target(
		${PROJECT_NAME}_CONFIG
		DEPENDS "${ViewerApp_BUILD_CONFIG_FILE}"
)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_CONFIG)

# --------------------------------
# Installation

# Install Data and Assets
install(DIRECTORY data;${OeCore_DATA_PATH};${OeScripting_DATA_PATH};${OeApp_DATA_PATH}
        DESTINATION bin
        )

# Install executable
install(TARGETS ${PROJECT_NAME}
        EXPORT ViewerAppTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
        )
install(
        EXPORT ViewerAppTargets
        FILE ViewerAppTargets.cmake
        NAMESPACE Oe::
        DESTINATION lib/cmake/${PROJECT_NAME}
        )

# Install PDB
include(OeUtils)
install_target_pdb(ViewerApp)

# Manually install the g3logger DLL from Orangine to this apps import directory
find_package(g3logger REQUIRED)
get_target_property(IMPORTED_CONFIGURATIONS_g3logger g3logger IMPORTED_CONFIGURATIONS)
get_target_property(IMPORTED_LOCATION_g3logger g3logger "IMPORTED_LOCATION_${IMPORTED_CONFIGURATIONS_g3logger}")

add_custom_command(TARGET ${PROJECT_NAME}
		POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy "${IMPORTED_LOCATION_g3logger}" "${PROJECT_BINARY_DIR}"
		)
install(FILES ${IMPORTED_LOCATION_g3logger}
        DESTINATION bin
)

# Config.cmake files
configure_file(ViewerAppConfig.cmake.in ViewerAppConfig.cmake @ONLY)
configure_file(ViewerAppConfigVersion.cmake.in ViewerAppConfigVersion.cmake @ONLY)
install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/ViewerAppConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/ViewerAppConfigVersion.cmake"
        DESTINATION lib/cmake/ViewerApp
)