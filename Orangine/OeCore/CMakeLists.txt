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
        src/JsonConfigReader.h
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
        src/Scene.cpp
        src/Scene_graph_manager.cpp
        src/Scene_graph_manager.h
        src/Shadowmap_manager.cpp
        src/Shadowmap_manager.h
        src/Skinned_mesh_component.cpp
        src/Skybox_material.cpp
        src/Test_component.cpp
        src/Texture.cpp
        src/Unlit_material.cpp
        src/VectorLog.cpp
        "src/Stubs/pch.h" 
        "src/D3D12/pch.h"
        "src/D3D12/D3D12_collision.cpp"
        "src/Collision.cpp"
        "ThirdParty/d3dx12/d3dx12.h"
        "ThirdParty/MikktSpace/mikktspace.h"
        "ThirdParty/MikktSpace/mikktspace.c"
        "ThirdParty/imgui/misc/cpp/imgui_stdlib.h"
        "ThirdParty/imgui/misc/cpp/imgui_stdlib.cpp"
        )
file(GLOB OeCore_HEADERS "include/OeCore/*")
list(APPEND OeCore_HEADERS "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h")

set_target_properties(OeCore PROPERTIES PUBLIC_HEADER "${OeCore_HEADERS}")
target_compile_features(OeCore PUBLIC cxx_std_17)
add_library(Oe::Core ALIAS OeCore)

# Precompiled Headers
include(PrecompiledHeader)
add_precompiled_header(${PROJECT_NAME} pch.h
        SOURCE_CXX
                src/pch.cpp
        IGNORE_FILES
                ThirdParty/imgui/misc/cpp/imgui_stdlib.cpp
                ThirdParty/MikktSpace/mikktspace.c
        )

target_link_libraries(${PROJECT_NAME} PUBLIC OeCompilerFlags)

# Public headers
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
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
#target_include_directories(${PROJECT_NAME} ${TinyGLTF_INCLUDE_DIR})

# Other 3P Libraries
target_link_libraries(${PROJECT_NAME} PUBLIC Oe3p::vectormath)

# DirectX
target_link_libraries(${PROJECT_NAME} PUBLIC
        d3d11.lib
        D3DCompiler.lib
        dxgi.lib
        dxguid.lib
        )

# DirectXTK
target_link_libraries(${PROJECT_NAME} PUBLIC DirectXTK12)

include(OeUtils)
install_target_pdb(OeCore)

# Data and Assets
set(${PROJECT_NAME}_DATA_SOURCE_DIRS "${PROJECT_NAME}=${PROJECT_SOURCE_DIR}/data")
set(${PROJECT_NAME}_DATA_SOURCE_DIRS "${${PROJECT_NAME}_DATA_SOURCE_DIRS}" PARENT_SCOPE)
install(DIRECTORY data
        DESTINATION bin
        )

# Config.cmake files
configure_file(OeCoreConfig.cmake.in OeCoreConfig.cmake @ONLY)
configure_file(OeCoreConfigVersion.cmake.in OeCoreConfigVersion.cmake @ONLY)
install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/OeCoreConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/OeCoreConfigVersion.cmake"
        DESTINATION lib/cmake/OeCore
)
