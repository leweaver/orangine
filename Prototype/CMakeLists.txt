project(Prototype VERSION 1.0
        DESCRIPTION "Orangine Sample Application"
        LANGUAGES CXX)

# Library Definition
add_executable(${PROJECT_NAME} WIN32
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
        uuid.lib
        kernel32.lib
        user32.lib
        gdi32.lib
        winspool.lib
        comdlg32.lib
        advapi32.lib
        shell32.lib
        ole32.lib
        oleaut32.lib
        odbc32.lib
        odbccp32.lib
        windowscodecs.lib
        )