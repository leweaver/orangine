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
        "ThirdParty/nv_helpers_dx12/TopLevelASGenerator.cpp"
        src/D3D12_input_manager.cpp
        src/D3D12_input_manager.h
        src/D3D12_entity_render_manager.cpp
        src/D3D12_entity_render_manager.h
        src/Gpu_buffer.cpp
        src/Gpu_buffer.h
        src/D3D12_material_manager.cpp
        src/D3D12_material_manager.h src/Descriptor_heap_pool.cpp src/Descriptor_heap_pool.h src/PipelineUtils.h src/D3D12_renderer_types.h src/D3D12_texture_manager.cpp src/D3D12_texture_manager.h src/Constant_buffer_pool.cpp src/Constant_buffer_pool.h src/Frame_resources.cpp src/Frame_resources.h src/Render_pass_copy_depth_to_resource.cpp src/Render_pass_copy_depth_to_resource.h src/D3D12_render_pass_shadow.cpp src/D3D12_render_pass_shadow.h)

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

# PIX
SET(DEP_DIRECTORY "${CMAKE_BINARY_DIR}/_deps")
SET(DEP_PIX_NAME WinPixEventRuntime)
SET(DEP_PIX_VERSION 1.0.220124001)
SET(DEP_PIX_DIRECTORY ${DEP_DIRECTORY}/${DEP_PIX_NAME}.${DEP_PIX_VERSION})
if(NOT IS_DIRECTORY ${DEP_PIX_DIRECTORY})
    # Download PIX if missing
    find_program(NUGET nuget)
    execute_process(COMMAND ${NUGET} install ${DEP_PIX_NAME} -OutputDirectory ${DEP_DIRECTORY} -Version ${DEP_PIX_VERSION})

    if(NOT IS_DIRECTORY ${DEP_PIX_DIRECTORY})
        MESSAGE(FATAL_ERROR "Failed to install PIX to ${DEP_PIX_DIRECTORY}")
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ${DEP_PIX_DIRECTORY}/bin/x64/WinPixEventRuntime.lib)
set(${PROJECT_NAME}_DYNAMIC_LIBRARIES
        "${DEP_PIX_DIRECTORY}/bin/x64/WinPixEventRunTime.dll"
        )
set(${PROJECT_NAME}_DYNAMIC_LIBRARIES ${${PROJECT_NAME}_DYNAMIC_LIBRARIES} PARENT_SCOPE)

# Set the DLL's on this target property so that the `oe_target_add_dependent_dynamic_libraries` helper will copy to the right places.
set_property(TARGET ${PROJECT_NAME} PROPERTY OE_DYNAMIC_LIBRARIES "${${PROJECT_NAME}_DYNAMIC_LIBRARIES}")

# DirectXTK
target_link_libraries(${PROJECT_NAME} PUBLIC DirectXTK12)

# DirectX Debug Interface
target_link_libraries(${PROJECT_NAME} PUBLIC dxgi.lib)

# Dependency Public headers
target_include_directories(${PROJECT_NAME}
        PUBLIC
        "${OE_THIRDPARTY_PATH}/DirectXTK/Inc"
        PRIVATE
        include/nv_helpers_dx12
        "${DEP_PIX_DIRECTORY}/Include/${DEP_PIX_NAME}"
        )

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