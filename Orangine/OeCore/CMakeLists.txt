project(OeCore VERSION 1.0
        DESCRIPTION "Orangine Core Library"
        LANGUAGES CXX)

configure_file (
        "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}Config.h.in"
        "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h"
)

# Library Definition
add_library(${PROJECT_NAME} STATIC
        src/pch.h
        src/pch.cpp
        src/User_interface_manager.h
        src/Shadowmap_manager.h
        src/Scene_graph_manager.h
        src/Render_step_manager.h
        src/Material_manager.h
        src/Entity_render_manager.h
        src/Dev_tools_manager.h
        src/D3D_resources_manager.h
        src/Animation_manager.h
        src/Asset_manager.h
        src/Animation_controller_component.cpp
        src/Animation_manager.cpp
        src/Asset_manager.cpp
        src/Camera_component.cpp
        src/Clear_gbuffer_material.cpp
        src/Component.cpp
        src/D3D_resources_manager.cpp
        src/Deferred_light_material.cpp
        src/Dev_tools_manager.cpp
        src/DeviceResources.cpp
        src/Entity.cpp
        src/Entity_graph_loader_gltf.cpp
		src/Entity_filter_impl.h
		src/Entity_filter_impl.cpp
        src/Entity_render_manager.cpp
        src/Entity_repository.cpp
        src/Entity_sorter.cpp
        src/Fps_counter.cpp
        src/Input_manager.cpp
        src/Light_component.cpp
        src/Light_provider.cpp
        src/Material.cpp
        src/Material_manager.cpp
        src/Mesh_data.cpp
        src/Mesh_data_component.cpp
        src/Mesh_utils.cpp
        src/Mesh_vertex_layout.cpp
        src/Mikk_tspace_triangle_mesh_interface.cpp
        src/Morph_weights_component.cpp
        src/PBR_material.cpp
        src/Primitive_mesh_data_factory.cpp
        src/Render_pass.cpp
        src/Render_pass_shadow.cpp
        src/Render_pass_skybox.cpp
        src/Render_step_manager.cpp
        src/Render_target_texture.cpp
        src/Renderable_component.cpp
        src/Renderer_data.cpp
        src/Renderer_enum.cpp
        src/Scene.cpp
        src/Scene_graph_manager.cpp
        src/Shadow_map_texture.cpp
        src/Shadow_map_texture_pool.cpp
        src/Shadowmap_manager.cpp
        src/Skinned_mesh_component.cpp
        src/Skybox_material.cpp
        src/Test_component.cpp
        src/Texture.cpp
        src/Unlit_material.cpp
        src/User_interface_manager.cpp
        src/EngineUtils.cpp
        src/VectorLog.cpp
    )
file(GLOB OeCore_HEADERS "include/OeCore/*")
list(APPEND OeCore_HEADERS "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h")

set_target_properties(OeCore PROPERTIES PUBLIC_HEADER "${OeCore_HEADERS}")
add_library(Oe::Core ALIAS OeCore)

# Precompiled Headers
include(PrecompiledHeader)
add_precompiled_header(${PROJECT_NAME} pch.h SOURCE_CXX src/pch.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC OeCompilerFlags)

# Public headers
get_filename_component(OE_THIRDPARTY_PATH "${Orangine_SOURCE_DIR}/../thirdparty" ABSOLUTE)
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        "${OE_THIRDPARTY_PATH}/tinygltf"
        "${OE_THIRDPARTY_PATH}/DirectXTK/Inc"
        )

# g3log
find_package(g3logger REQUIRED)
target_include_directories(${PROJECT_NAME}
        PUBLIC ${G3LOG_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC g3logger)

# Third Party (imgui, MikktSpace)
find_package(OeThirdParty REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Oe::MikktSpace Oe::OeImgui)

# DirectX
target_link_libraries(${PROJECT_NAME} PUBLIC
        d3d11.lib
        D3DCompiler.lib
        dxgi.lib
        dxguid.lib
        )

# DirectXTK
find_package(directxtk REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC
        "${DIRECTX_LIB_DIR}/Desktop_2017_Win10/x64/${CMAKE_BUILD_TYPE}/DirectXTK.lib")

# Install
install(TARGETS ${PROJECT_NAME}
        EXPORT OeCoreTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
        )

include(OeUtils)
install_target_pdb(OeCore)

install(
        EXPORT OeCoreTargets
        FILE OeCoreTargets.cmake
        NAMESPACE Oe::
        DESTINATION lib/cmake/${PROJECT_NAME}
        )
		
install(DIRECTORY data
        DESTINATION bin
        )

# Install Python binaries
install(FILES "${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}_d.dll"
		CONFIGURATIONS Debug
		DESTINATION bin)
install(FILES "${Python3_RUNTIME_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.dll"
		CONFIGURATIONS Release
		DESTINATION bin)

# Config.cmake files
configure_file(OeCoreConfig.cmake.in OeCoreConfig.cmake @ONLY)
configure_file(OeCoreConfigVersion.cmake.in OeCoreConfigVersion.cmake @ONLY)
install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/OeCoreConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/OeCoreConfigVersion.cmake"
        DESTINATION lib/cmake/OeCore
)
