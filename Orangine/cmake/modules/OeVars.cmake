
if ("${CMAKE_SIZEOF_VOID_P}" MATCHES "4")
    set (Orangine_ARCHITECTURE x86)
elseif ("${CMAKE_SIZEOF_VOID_P}" MATCHES "8")
    set (Orangine_ARCHITECTURE x64)
else ()
    MESSAGE(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif ()

# If the build type is RelWithDebInfo, link with 3rd party dependencies using Release.
if(${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
    set(OE_DEPENDENCY_BUILD_TYPE "Release")
else()
    set(OE_DEPENDENCY_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()

list(APPEND CMAKE_PREFIX_PATH "${Orangine_SOURCE_DIR}/../bin/${Orangine_ARCHITECTURE}/${OE_DEPENDENCY_BUILD_TYPE}/lib/cmake")
list(APPEND CMAKE_PREFIX_PATH "${Orangine_SOURCE_DIR}/../bin/${Orangine_ARCHITECTURE}/${OE_DEPENDENCY_BUILD_TYPE}/share/cmake")

# If this check fails, you need to provide CMAKE_SYSTEM_VERSION on the command line, then delete and regenerate your cmake cache. 
# See: https://gitlab.kitware.com/cmake/cmake/-/issues/16713
set(OE_EXPECTED_WINSDK_VER "10.0.19041.0")
if (NOT "${CMAKE_SYSTEM_VERSION}" MATCHES "${OE_EXPECTED_WINSDK_VER}")
     message(FATAL_ERROR "CMAKE_SYSTEM_VERSION is '${CMAKE_SYSTEM_VERSION}', but must be '${OE_EXPECTED_WINSDK_VER}'")
endif()

# Python search paths
if ("${Orangine_ARCHITECTURE}" MATCHES "x86")
        set(ENV{VIRTUAL_ENV} "${OE_THIRDPARTY_PATH}/pyenv_37_x86")
elseif ("${Orangine_ARCHITECTURE}" MATCHES "x64")
        set(ENV{VIRTUAL_ENV} "${OE_THIRDPARTY_PATH}/pyenv_37_x64")
else ()
        MESSAGE(ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif ()
set(Python_FIND_VIRTUALENV ONLY)
set(Python3_FIND_VIRTUALENV ONLY)
