cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# Hide the 'Up-to-date' install messages
set(CMAKE_INSTALL_MESSAGE LAZY)

# Helper file locations
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/cmake;${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

project(Orangine VERSION 1.0
                 DESCRIPTION "Orangine - realtime glTF rendering engine"
                 LANGUAGES CXX)

# --------------------------------
# Build options
# --------------------------------
option(OE_BUILD_ASAN "Add address sanitization, if compiler supports." FALSE)
option(OE_BUILD_TESTING "Enable unit test targets" FALSE)
option(OE_BUILD_SAMPLES "Enable sample app targets" TRUE)

# --------------------------------
# Conan Dependencies
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan.cmake"
            EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
            TLS_VERIFY ON)
endif()
include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_configure(REQUIRES g3log/1.3.3 pybind11/2.7.1 imgui/1.85 yaml-cpp/0.7.0 utfcpp/3.2.1 gtest/1.11.0 ms-gsl/3.1.0 range-v3/0.11.0
        GENERATORS cmake_find_package
        BUILD_REQUIRES cmake/3.20.5
        OPTIONS g3log:shared=False)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
        BUILD missing
        REMOTE conancenter
        SETTINGS ${settings})

# --------------------------------
# NuGet - Check it is installed on the command line.
find_program(NUGET nuget)
if(NOT NUGET)
    message(FATAL_ERROR "Cannot find nuget command line tool.\nInstall it with e.g. choco install nuget.commandline")
endif()

# --------------------------------
# Orangine Helper Modules. Ordering matters here.
include(OeThirdPartyContent)
include(OeVars)
include(OeCompilerFlags)
include(OePython)
include(OeUtils)

# --------------------------------
# Sub-projects

# Engine
add_subdirectory(OeCore)
add_subdirectory(OePipelineD3D12)
add_subdirectory(OeScripting)
add_subdirectory(OeApp)
# Samples
if (${OE_BUILD_SAMPLES})
add_subdirectory(ViewerApp)
endif()

# --------------------------------
# Install
install(TARGETS vectormath OeCore OeScripting OeApp OePipelineD3D12
        EXPORT OrangineTargets
        INCLUDES DESTINATION include
        PUBLIC_HEADER DESTINATION include
        )
install(
        EXPORT OrangineTargets
        FILE OrangineTargets.cmake
        NAMESPACE Oe::
        DESTINATION lib/cmake/${PROJECT_NAME}
        )