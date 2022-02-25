if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set (Orangine_ARCHITECTURE x86)
elseif ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
    set (Orangine_ARCHITECTURE x64)
else ()
    MESSAGE(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif ()

# If the build type is RelWithDebInfo, link with 3rd party dependencies using Release.
if(${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
    set(OE_DEPENDENCY_BUILD_TYPE "Release")
else()
    set(OE_DEPENDENCY_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()

list(APPEND CMAKE_PREFIX_PATH "${Orangine_SOURCE_DIR}/../bin/${Orangine_ARCHITECTURE}/${OE_DEPENDENCY_BUILD_TYPE}/lib/cmake")
list(APPEND CMAKE_PREFIX_PATH "${Orangine_SOURCE_DIR}/../bin/${Orangine_ARCHITECTURE}/${OE_DEPENDENCY_BUILD_TYPE}/share/cmake")

# If this check fails, you need to provide CMAKE_SYSTEM_VERSION on the command line, then delete and regenerate your cmake cache. 
# See: https://gitlab.kitware.com/cmake/cmake/-/issues/16713
if ("${CMAKE_SYSTEM_VERSION}" STREQUAL "")
     message(FATAL_ERROR "CMAKE_SYSTEM_VERSION is not set.")
endif()

set (OE_WINSDK_DIR "$ENV{ProgramFiles\(x86\)}\\Windows Kits\\10\\Include\\${CMAKE_SYSTEM_VERSION}")
if (NOT EXISTS "${OE_WINSDK_DIR}")
     MESSAGE(WARNING "Windows SDK '${CMAKE_SYSTEM_VERSION}' not installed. Expected location: ${OE_WINSDK_DIR}")
endif()
