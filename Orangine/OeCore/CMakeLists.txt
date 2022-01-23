project(OeCore VERSION 1.0
        DESCRIPTION "Orangine Core Library"
        LANGUAGES CXX C)

configure_file (
        "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}Config.h.in"
        "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h"
)

# Library Definition
add_library(${PROJECT_NAME} STATIC
        src/pch.h
        src/pch.cpp
        src/Animation_controller_component.cpp
        src/Animation_manager.cpp
        src/Animation_manager.h
        src/Asset_manager.cpp
        src/Asset_manager.h
        src/Behavior_manager.cpp
        src/Camera_component.cpp
        src/Clear_gbuffer_material.cpp
        src/Color.cpp
        src/Component.cpp
        # src/D3D11/D3D_collision.cpp
        # src/D3D11/D3D_device_repository.cpp
        # src/D3D11/D3D_device_repository.h
        # src/D3D11/D3D_entity_render_manager.cpp
        # src/D3D11/D3D_material_manager.cpp
        # src/D3D11/D3D_material_manager.h
        # src/D3D11/D3D_render_pass_shadow.cpp
        # src/D3D11/D3D_render_step_manager.cpp
        # src/D3D11/D3D_render_step_manager.h
        # src/D3D11/D3D_renderer_data.cpp
        # src/D3D11/D3D_renderer_data.h
        # src/D3D11/D3D_texture_manager.cpp
        # src/D3D11/D3D_texture_manager.h
        # src/D3D11/D3D_directX_utils.h
        # src/D3D11/D3D_user_interface_manager.cpp
        # src/D3D11/D3D_user_interface_manager.h
        "src/D3D12/D3D12_device_resources.cpp"
        "src/D3D12/D3D12_vendor.h"
        src/Stubs/Stub_managers.cpp
        src/Deferred_light_material.cpp
        src/Dev_tools_manager.cpp
        src/Dev_tools_manager.h
        src/EngineUtils.cpp
        src/Entity.cpp
        src/Entity_filter_impl.cpp
        src/Entity_filter_impl.h
        src/Entity_graph_loader_gltf.cpp
        src/Entity_render_manager.cpp
        src/Entity_render_manager.h
        src/Entity_repository.cpp
        src/Entity_sorter.cpp
        src/Fps_counter.cpp
        src/Input_manager.cpp
        src/JsonConfigReader.cpp
        src/Light_component.cpp
        src/Light_provider.cpp
        src/Material.cpp
        src/Material_manager.cpp
        src/Material_manager.h
        src/Math_constants.cpp
        src/Mesh_data.cpp
        src/Mesh_data_component.cpp
        src/Mesh_utils.cpp
        src/Mesh_vertex_layout.cpp
        src/Mikk_tspace_triangle_mesh_interface.cpp
        src/Morph_weights_component.cpp
        src/PBR_material.cpp
        src/Primitive_mesh_data_factory.cpp
        src/Render_pass.cpp
        src/Render_pass_skybox.cpp
        src/Render_step_manager.cpp
        src/Renderable_component.cpp
        src/Renderer_data.cpp
        src/Renderer_enums.cpp
        src/Scene_graph_manager.cpp
        src/Scene_graph_manager.h
        src/Shadowmap_manager.cpp
        src/Shadowmap_manager.h
        src/Skinned_mesh_component.cpp
        src/Skybox_material.cpp
        src/Test_component.cpp
        src/Texture.cpp
        src/Time_step_manager.cpp
        src/Time_step_manager.h
        src/Unlit_material.cpp
        src/VectorLog.cpp
        "src/Stubs/pch.h" 
        "src/D3D12/pch.h"
        "src/D3D12/D3D12_collision.cpp"
        "src/D3D12/D3D12_lighting_manager.cpp"
        "src/D3D12/D3D12_lighting_manager.h"
        "src/D3D12/D3D12_renderer_data.h"
        "src/Collision.cpp"
        "src/Statics.cpp"
        "src/OeCore.cpp"
        "ThirdParty/d3dx12/d3dx12.h"
        "ThirdParty/MikktSpace/mikktspace.h"
        "ThirdParty/MikktSpace/mikktspace.c"
        "ThirdParty/imgui/misc/cpp/imgui_stdlib.h"
        "ThirdParty/imgui/misc/cpp/imgui_stdlib.cpp"
        "ThirdParty/nv_helpers_dx12/BottomLevelASGenerator.cpp"
        "ThirdParty/nv_helpers_dx12/BottomLevelASGenerator.h"
        "ThirdParty/nv_helpers_dx12/RaytracingPipelineGenerator.cpp"
        "ThirdParty/nv_helpers_dx12/RaytracingPipelineGenerator.h"
        "ThirdParty/nv_helpers_dx12/RootSignatureGenerator.cpp"
        "ThirdParty/nv_helpers_dx12/RootSignatureGenerator.h"
        "ThirdParty/nv_helpers_dx12/ShaderBindingTableGenerator.cpp"
        "ThirdParty/nv_helpers_dx12/ShaderBindingTableGenerator.h"
        "ThirdParty/nv_helpers_dx12/TopLevelASGenerator.cpp"
        "ThirdParty/nv_helpers_dx12/TopLevelASGenerator.h"
        src/D3D12/D3D12_render_step_manager.cpp src/D3D12/D3D12_render_step_manager.h)

# CPP17
target_compile_features(OeCore PUBLIC cxx_std_17)
if (MSVC)
    # There is a bug in visual studio that prevents intellisense from realizing
    # /std:c++17 is on the command line if you only use target_compile_features(cxx_std_17)
    target_compile_options(${PROJECT_NAME} PUBLIC "/std:c++17")
endif()

add_library(Oe::Core ALIAS OeCore)

#####
# Dependencies
#####

target_link_libraries(${PROJECT_NAME} PUBLIC OeCompilerFlags)

# Public headers
target_include_directories(${PROJECT_NAME}
        PUBLIC
        "${OE_THIRDPARTY_PATH}/filewatch"
        "${OE_THIRDPARTY_PATH}/DirectXTK/Inc"
        PRIVATE
        ThirdParty
        )

# g3log
find_package(g3log REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC g3log::g3log)

# imgui
find_package(imgui REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui::imgui)

# tinygltf
target_link_libraries(${PROJECT_NAME} PUBLIC Oe3p::tinygltf)

# vectormath
target_link_libraries(${PROJECT_NAME} PUBLIC Oe3p::vectormath)

# utf8
find_package(utf8cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC utf8cpp)

# DirectX
target_link_libraries(${PROJECT_NAME} PUBLIC
        d3d11.lib
        D3DCompiler.lib
        dxgi.lib
        dxguid.lib
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

# register the yaml file
oe_target_add_config_yaml(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/src/config.default.yaml)

# Precompiled Headers
include(PrecompiledHeader)
add_precompiled_header(${PROJECT_NAME} pch.h
        SOURCE_CXX
        src/pch.cpp
        IGNORE_FILES
        ThirdParty/imgui/misc/cpp/imgui_stdlib.cpp
        ThirdParty/MikktSpace/mikktspace.c
        ThirdParty/nv_helpers_dx12/BottomLevelASGenerator.cpp
        ThirdParty/nv_helpers_dx12/RaytracingPipelineGenerator.cpp
        ThirdParty/nv_helpers_dx12/RootSignatureGenerator.cpp
        ThirdParty/nv_helpers_dx12/ShaderBindingTableGenerator.cpp
        ThirdParty/nv_helpers_dx12/TopLevelASGenerator.cpp
        )

# Make sure to include PDB files when installing in debug mode
oe_target_install_pdb(${PROJECT_NAME})

#####
# Enums
#####
message("${Python3_EXECUTABLE}")
oe_target_generate_enum(${PROJECT_NAME}
        SOURCE "${PROJECT_SOURCE_DIR}/codegen/Renderer_enums.json"
        BYPRODUCTS include/${PROJECT_NAME}/Renderer_enums.h
)