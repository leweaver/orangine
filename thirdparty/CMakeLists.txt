cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(OeThirdParty VERSION 1.0
        DESCRIPTION "Orangine ThirdParty Library Wrappers"
        LANGUAGES CXX C)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        MESSAGE("Setting path")
        SET(CMAKE_INSTALL_PREFIX "$ENV{LOCALAPPDATA}/Orangine" CACHE PATH Orangine FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
# include(OeUtils)

add_compile_definitions("_UNICODE;UNICODE;WIN32;_WINDOWS;_LIB;HAVE_SNPRINTF")
if(CMAKE_BUILD_TYPE MATCHES Debug)
        add_compile_definitions("_DEBUG")
endif()

# MikktSpace library
add_subdirectory(MikktSpace)

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

install(TARGETS DirectXHelpers
        EXPORT OeThirdPartyTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static
        INCLUDES DESTINATION include
        PUBLIC_HEADER DESTINATION include/OeThirdParty
)

install(
        EXPORT OeThirdPartyTargets
        FILE OeThirdPartyTargets.cmake
        NAMESPACE Oe::
        DESTINATION "lib/cmake/${PROJECT_NAME}"
)

# Install PDB's
# install_target_pdb(MikktSpace)

# Version
configure_file(OeThirdPartyConfig.cmake.in OeThirdPartyConfig.cmake @ONLY)
configure_file(OeThirdPartyConfigVersion.cmake.in OeThirdPartyConfigVersion.cmake @ONLY)
install(
        FILES
                "${CMAKE_CURRENT_BINARY_DIR}/OeThirdPartyConfig.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/OeThirdPartyConfigVersion.cmake"
        DESTINATION "lib/cmake/${PROJECT_NAME}"
)
