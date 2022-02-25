project(OePipelineD3D12 VERSION 1.0
        DESCRIPTION "Orangine DirectX12 Pipeline Library"
        LANGUAGES CXX C)

configure_file (
        "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}Config.h.in"
        "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h"
)

# Library Definition
add_library(${PROJECT_NAME} STATIC
        "include/${PROJECT_NAME}/${PROJECT_NAME}.h"
        "include/${PROJECT_NAME}/Statics.h"
        "src/${PROJECT_NAME}.cpp"
        "src/Statics.cpp"
        "src/D3D12_device_resources.cpp"
        "src/D3D12_vendor.h"
        "src/D3D12_collision.cpp"
        "src/D3D12_lighting_manager.cpp"
        "src/D3D12_lighting_manager.h"
        "src/D3D12_renderer_data.h"
        "src/D3D12_render_step_manager.cpp"
        "src/D3D12_render_step_manager.h"
        "src/Primitive_mesh_data_factory.h"
        "src/Primitive_mesh_data_factory.cpp"
        "ThirdParty/nv_helpers_dx12/BottomLevelASGenerator.cpp"
        "ThirdParty/nv_helpers_dx12/RaytracingPipelineGenerator.cpp"
        "ThirdParty/nv_helpers_dx12/RootSignatureGenerator.cpp"
        "ThirdParty/nv_helpers_dx12/ShaderBindingTableGenerator.cpp"
        "ThirdParty/nv_helpers_dx12/TopLevelASGenerator.cpp")

# CPP17
target_compile_features(OeCore PUBLIC cxx_std_17)
if (MSVC)
    # There is a bug in visual studio that prevents intellisense from realizing
    # /std:c++17 is on the command line if you only use target_compile_features(cxx_std_17)
    target_compile_options(${PROJECT_NAME} PUBLIC "/std:c++17")
endif()

add_library(Oe::PipelineD3D12 ALIAS ${PROJECT_NAME})

oe_target_generate_export_header(${PROJECT_NAME})

#####
# Dependencies
#####

target_link_libraries(${PROJECT_NAME} PUBLIC OeCore)

# Public headers
target_include_directories(${PROJECT_NAME}
        PUBLIC
        "${OE_THIRDPARTY_PATH}/DirectXTK/Inc"
        PRIVATE
        include/nv_helpers_dx12
        )

# DirectXTK
target_link_libraries(${PROJECT_NAME} PUBLIC DirectXTK12)

#####
# Orangine module configuration
#####

# configure /data folder
oe_target_add_data_dir(${PROJECT_NAME})

# configure /cmake folder and src/{PROJECT_NAME}Config.h.in file
oe_target_configure_cmake_dir(${PROJECT_NAME})

# configure /include folder (public header files) and configure .cmake files
oe_target_configure_include_dir(${PROJECT_NAME})

# Precompiled Headers - Only for C++ as WRL cannot be included into C.
target_precompile_headers(${PROJECT_NAME}
        PUBLIC
        $<$<COMPILE_LANGUAGE:CXX>:<OeCore/OeCore.h$<ANGLE-R>>
        PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:
        <OeCore/WindowsDefines.h$<ANGLE-R>>
        )
set_target_properties(${PROJECT_NAME} PROPERTIES
        SKIP_PRECOMPILE_HEADERS
        "ThirdParty/nv_helpers_dx12/BottomLevelASGenerator.cpp;ThirdParty/nv_helpers_dx12/RaytracingPipelineGenerator.cpp;ThirdParty/nv_helpers_dx12/RootSignatureGenerator.cpp;ThirdParty/nv_helpers_dx12/ShaderBindingTableGenerator.cpp;ThirdParty/nv_helpers_dx12/TopLevelASGenerator.cpp")

# Make sure to include PDB files when installing in debug mode
oe_target_install_pdb(${PROJECT_NAME})