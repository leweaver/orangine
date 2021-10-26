
set(OE_DEPENDENCIES_VECTORMATH "FETCHCONTENT" CACHE STRING "Location where to find vectormath modules. can be [INSTALLED|FETCHCONTENT]")
set(OE_DEPENDENCIES_VECTORMATH_INSTALLED INSTALLED)
set(OE_DEPENDENCIES_VECTORMATH_FETCHCONTENT FETCHCONTENT)

set(OE_DEPENDENCIES_TINYGLTF "EXTERNAL" CACHE STRING "Location where to find vectormath modules. can be [INSTALLED|EXTERNAL]")
set(OE_DEPENDENCIES_TINYGLTF_INSTALLED INSTALLED)
set(OE_DEPENDENCIES_TINYGLTF_EXTERNAL EXTERNAL)

set(OE_DEPENDENCIES_DIRECTXTK12 "FETCHCONTENT" CACHE STRING "Location where to find DirectXTK12 modules. can be [INSTALLED|FETCHCONTENT]")
set(OE_DEPENDENCIES_DIRECTXTK12_INSTALLED INSTALLED)
set(OE_DEPENDENCIES_DIRECTXTK12_FETCHCONTENT FETCHCONTENT)

################################
# Dependencies
include(FetchContent)
include(ExternalProject)
set_directory_properties(PROPERTIES EP_UPDATE_DISCONNECTED true)

FetchContent_Declare(
        vectormath
        GIT_REPOSITORY git@github.com:glampert/vectormath.git
        GIT_TAG ee960fad0a4bbbf0ee2e7d03fc749c49ebeefaef
)
FetchContent_Declare(
        tinygltf
        GIT_REPOSITORY git@github.com:syoyo/tinygltf.git
        GIT_TAG a159945db9d97e79a30cb34e2aaa45fd28dea576
)
FetchContent_Declare(
        DirectXTK12
        GIT_REPOSITORY git@github.com:Microsoft/DirectXTK12.git
        GIT_TAG nov2020b
)

SET(tinygltf_ROOT ${PROJECT_BINARY_DIR}/tinygltf)
ExternalProject_Add(
        tinygltf_external
        GIT_REPOSITORY git@github.com:syoyo/tinygltf.git
        GIT_TAG a159945db9d97e79a30cb34e2aaa45fd28dea576
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${tinygltf_ROOT}
        CMAKE_CACHE_ARGS -DTINYGLTF_BUILD_LOADER_EXAMPLE:BOOL=OFF -DTINYGLTF_BUILD_GL_EXAMPLES:BOOL=OFF -DTINYGLTF_BUILD_VALIDATOR_EXAMPLE:BOOL=OFF
)

# vectormath
if(OE_DEPENDENCIES_VECTORMATH STREQUAL OE_DEPENDENCIES_VECTORMATH_FETCHCONTENT)
    message(STATUS "Finding vectormath in location=FETCHCONTENT")
    FetchContent_MakeAvailable(vectormath)

    add_library(vectormath INTERFACE)
    add_library(Oe3p::vectormath ALIAS vectormath)
    target_include_directories(vectormath
            INTERFACE
            $<BUILD_INTERFACE:${vectormath_SOURCE_DIR}>
            $<INSTALL_INTERFACE:include>
            )
endif()

# tinygltf
if(OE_DEPENDENCIES_TINYGLTF STREQUAL OE_DEPENDENCIES_TINYGLTF_EXTERNAL)
    add_library(tinygltf INTERFACE IMPORTED)
    add_dependencies(tinygltf tinygltf_external)
    add_library(Oe3p::tinygltf ALIAS tinygltf)
    target_include_directories(tinygltf
            INTERFACE
            $<BUILD_INTERFACE:${tinygltf_ROOT}/include>
            $<INSTALL_INTERFACE:include>
            )
endif()

# DirectXTK12
if(OE_DEPENDENCIES_DIRECTXTK12 STREQUAL OE_DEPENDENCIES_DIRECTXTK12_FETCHCONTENT)
    message(STATUS "Finding DirectXTK12 in location=FETCHCONTENT")
    FetchContent_MakeAvailable(DirectXTK12)
endif()

# TODO: Check this out for a cool widget
# https://github.com/CedricGuillemet/ImGuizmo