if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR
   CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
        set(OE_COMPILER_IS_MSVC 1)
endif()

#-----------------------------------------------------------------------------
# oe_compiler_flags is used by all the oe targets and consumers of Orangine
# The flags on oe_compiler_flags are needed when using/building Orangine
add_library(OeCompilerFlags INTERFACE)

if(OE_COMPILER_IS_MSVC)
        add_compile_definitions("_UNICODE;UNICODE;WIN32;_WINDOWS;_LIB;HAVE_SNPRINTF;_ENABLE_EXTENDED_ALIGNED_STORAGE")

        add_compile_options(/EHsc /Gd /GS /FC /diagnostics:column)

        # ASan support in VS2019 (142) and above
        #if ("${MSVC_TOOLSET_VERSION}" GREATER "141")
        #        add_compile_options(/fsanitize=address)
        #endif()

        if(CMAKE_BUILD_TYPE MATCHES Debug)
                add_compile_definitions("_DEBUG")
        endif()
endif()



include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/OeCompilerFlagsConfigVersion.cmake"
    VERSION 1.0
    COMPATIBILITY AnyNewerVersion
)

# Install
install(TARGETS OeCompilerFlags
        EXPORT OeCompilerFlagsTargets

        )


include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/OeCompilerFlagsConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/OeCompilerFlagsConfig.cmake"
    INSTALL_DESTINATION lib/cmake/OeCompilerFlags
)

install(EXPORT OeCompilerFlagsTargets DESTINATION lib/cmake/OeCompilerFlags)
install(FILES "${PROJECT_BINARY_DIR}/OeCompilerFlagsConfigVersion.cmake"
              "${PROJECT_BINARY_DIR}/OeCompilerFlagsConfig.cmake"
        DESTINATION lib/cmake/OeCompilerFlags)
