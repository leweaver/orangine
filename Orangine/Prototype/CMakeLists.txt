project(Prototype VERSION 1.0
        DESCRIPTION "Orangine Sample Application"
        LANGUAGES CXX)

set(Prototype_DATA_PATH ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/data)
set(Prototype_THIRDPARTY_PATH ${CMAKE_SOURCE_DIR}/../thirdparty)

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

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        )
install(DIRECTORY data
        DESTINATION bin
        )

# Precompiled Headers
# [=[
include(PrecompiledHeader)
add_precompiled_header(${PROJECT_NAME} pch.h
        SOURCE_CXX src/pch.cpp)
#]=]

target_link_libraries(${PROJECT_NAME} PUBLIC OeCompilerFlags)

target_link_libraries(${PROJECT_NAME} PRIVATE Oe::Core)

target_link_libraries(${PROJECT_NAME} PRIVATE
        WindowsApp.lib
        # odbc32.lib
        # odbccp32.lib
        # windowscodecs.lib
        )
        
set_property(TARGET Prototype PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

install(TARGETS Prototype
        EXPORT PrototypeTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )