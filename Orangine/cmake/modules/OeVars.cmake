
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

# Python search paths
if ("${Orangine_ARCHITECTURE}" MATCHES "x86")
        set(Python3_ROOT_DIR "$ENV{LOCALAPPDATA}/Programs/Python/Python37-32/")
elseif ("${Orangine_ARCHITECTURE}" MATCHES "x64")
        set(Python3_ROOT_DIR "$ENV{LOCALAPPDATA}/Programs/Python/Python37/")
else ()
        MESSAGE(ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif ()
set(Python3_FIND_REGISTRY NEVER)
