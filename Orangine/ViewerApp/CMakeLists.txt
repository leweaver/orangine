project(ViewerApp VERSION 1.0
        DESCRIPTION "Orangine Sample Application"
        LANGUAGES CXX)

set(ViewerApp_DATA_PATH ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/data)
set(ViewerApp_THIRDPARTY_PATH ${CMAKE_SOURCE_DIR}/../thirdparty)

configure_file (
        "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}Config.h.in"
        "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h"
)

# Library Definition
add_executable(${PROJECT_NAME} WIN32
        ${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h
        resource.rc
        resource.aps
        settings.manifest
        src/pch.h
        src/pch.cpp
        src/Game.h
        src/Game.cpp
        src/Main.cpp
        src/LogFileSink.h
        src/LogFileSink.cpp
        src/VectorLogSink.h
        src/VectorLogSink.cpp
        src/filesinkhelper.ipp)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        "${PROJECT_BINARY_DIR}/include/"
)

# Precompiled Headers
include(PrecompiledHeader)
add_precompiled_header(${PROJECT_NAME} pch.h
        SOURCE_CXX src/pch.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC OeCompilerFlags)

target_link_libraries(${PROJECT_NAME} PRIVATE Oe::Core)

target_link_libraries(${PROJECT_NAME} PRIVATE WindowsApp.lib)

set_property(TARGET ViewerApp PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# Install data and exe
install(DIRECTORY data
        DESTINATION bin
        )
install(TARGETS ${PROJECT_NAME}
        EXPORT ViewerAppTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )

# Manually install the g3logger DLL from Orangine to this apps import directory
find_package(g3logger REQUIRED)
get_target_property(IMPORTED_CONFIGURATIONS_g3logger g3logger IMPORTED_CONFIGURATIONS)
get_target_property(IMPORTED_LOCATION_g3logger g3logger "IMPORTED_LOCATION_${IMPORTED_CONFIGURATIONS_g3logger}")

add_custom_command(TARGET ${PROJECT_NAME}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy "${IMPORTED_LOCATION_g3logger}" "${PROJECT_BINARY_DIR}"
		)
install(FILES ${IMPORTED_LOCATION_g3logger}
        DESTINATION bin
)

# Manually copy the python DLL for builds, to keep the Py_SetPythonHome path < 260 characters.
find_package(Python3 COMPONENTS Development)
set(Python3_DLL_NAME "python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}$<$<CONFIG:debug>:_d>$<$<CONFIG:release>:>.dll")
message("Python3_DLL_NAME=${Python3_DLL_NAME}")
add_custom_command(TARGET ${PROJECT_NAME}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy "${Python3_RUNTIME_LIBRARY_DIRS}/${Python3_DLL_NAME}" "${PROJECT_BINARY_DIR}"
		)

# Install PDB
include(OeUtils)
install_target_pdb(ViewerApp)
