if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR
   CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
        set(OE_COMPILER_IS_MSVC 1)
endif()

#-----------------------------------------------------------------------------
# oe_compiler_flags is used by all the oe targets and consumers of Orangine
# The flags on oe_compiler_flags are needed when using/building Orangine
add_library(OeCompilerFlags INTERFACE)

# setup that we need C++17 support
target_compile_features(OeCompilerFlags INTERFACE cxx_std_17)

if(OE_COMPILER_IS_MSVC)
        if(NOT DEFINED OE_WINSDK_PATH)
                set(OE_WINSDK_VERSION 10.0.17134.0)
        endif()

        # SDK Locations
        # HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows\v10.0
        if(NOT DEFINED OE_WINSDK_PATH)
                GET_FILENAME_COMPONENT(OE_WINSDK_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0;InstallationFolder]" ABSOLUTE CACHE)
                set(OE_WINSDK_PATH "${OE_WINSDK_PATH}/Include/${OE_WINSDK_VERSION}")
        endif()

        # HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7
        if(NOT DEFINED OE_MSVC_INCLUDE)
            GET_FILENAME_COMPONENT(OE_MSVC_INCLUDE "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\VisualStudio\\SxS\\VS7;15.0]" REALPATH)
        endif()

        add_compile_definitions("_UNICODE;UNICODE;WIN32;_WINDOWS;_LIB;HAVE_SNPRINTF")

        if(CMAKE_BUILD_TYPE MATCHES Debug)
                add_compile_definitions("_DEBUG")
        endif()
endif()


# Install
install(TARGETS OeCompilerFlags
        EXPORT OeCompilerFlagsTargets
        )

install(
        EXPORT OeCompilerFlagsTargets
        FILE OeCompilerFlagsTargets.cmake
        NAMESPACE Oe:: 
        DESTINATION lib/cmake/OeCompilerFlags
        )