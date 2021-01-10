cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(OeThirdParty VERSION 1.0
        DESCRIPTION "Orangine ThirdParty Library Wrappers"
        LANGUAGES CXX C)

# Update module path
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../Orangine/cmake/modules")
include(OeUtils)

add_compile_definitions("_UNICODE;UNICODE;WIN32;_WINDOWS;_LIB;HAVE_SNPRINTF")
if(CMAKE_BUILD_TYPE MATCHES Debug)
        add_compile_definitions("_DEBUG")
endif()

# ImGui library
add_library(OeImgui STATIC
        "imgui/imgui/examples/imgui_impl_dx11.cpp"
        "imgui/imgui/examples/imgui_impl_win32.cpp"
        "imgui/imgui/imgui.cpp"
        "imgui/imgui/imgui_demo.cpp"
        "imgui/imgui/imgui_draw.cpp"
        "imgui/imgui/imgui_widgets.cpp"
        "imgui/imgui/misc/cpp/imgui_stdlib.cpp"
)
set(OeImgui_HEADERS
        "imgui/imgui/examples/imgui_impl_dx11.h"
        "imgui/imgui/examples/imgui_impl_win32.h"
        "imgui/imgui/imconfig.h"
        "imgui/imgui/imgui.h"
        "imgui/imgui/imgui_internal.h"
        "imgui/imgui/imstb_rectpack.h"
        "imgui/imgui/imstb_truetype.h"
        "imgui/imgui/misc/cpp/imgui_stdlib.h"
)
set_target_properties(OeImgui PROPERTIES PUBLIC_HEADER "${OeImgui_HEADERS}")

add_library(Oe::Imgui ALIAS OeImgui)
target_include_directories(OeImgui
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        "imgui/imgui"
        )
target_link_libraries(OeImgui PUBLIC OeCompilerFlags)

# MikktSpace library
add_library(MikktSpace STATIC
        "MikktSpace/src/mikktspace.c"
        )
set_target_properties(MikktSpace PROPERTIES
        PUBLIC_HEADER "MikktSpace/include/MikktSpace/mikktspace.h"
)
add_library(Oe::MikktSpace ALIAS MikktSpace)
target_include_directories(MikktSpace
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        "MikktSpace/include/MikktSpace"
        )
target_link_libraries(OeImgui PUBLIC OeCompilerFlags)

# DirectXHelpers library
add_library(DirectXHelpers INTERFACE)
set_target_properties(DirectXHelpers PROPERTIES
        PUBLIC_HEADER "DirectXHelpers/include/d3dx12.h"
)
add_library(Oe::DirectXHelpers ALIAS DirectXHelpers)
target_include_directories(DirectXHelpers
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )
target_link_libraries(OeImgui PUBLIC OeCompilerFlags)

install(TARGETS OeImgui MikktSpace DirectXHelpers
    EXPORT OeThirdPartyTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
    PUBLIC_HEADER DESTINATION include/OeThirdParty
)

install(
    EXPORT OeThirdPartyTargets
    FILE OeThirdPartyTargets.cmake
    NAMESPACE Oe::
    DESTINATION lib/cmake/OeThirdParty)

# Install PDB's
install_target_pdb(OeImgui)
install_target_pdb(MikktSpace)

# Version
configure_file(OeThirdPartyConfig.cmake.in OeThirdPartyConfig.cmake @ONLY)
configure_file(OeThirdPartyConfigVersion.cmake.in OeThirdPartyConfigVersion.cmake @ONLY)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/OeThirdPartyConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/OeThirdPartyConfigVersion.cmake"
    DESTINATION lib/cmake/OeThirdParty
)
