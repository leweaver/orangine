project(OeCore VERSION 1.0
        DESCRIPTION "Orangine Core Library"
        LANGUAGES CXX C)

configure_file (
        "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}Config.h.in"
        "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}Config.h"
)

# Library Definition
add_library(${PROJECT_NAME} STATIC
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
        src/Entity_repository.cpp
        src/Entity_sorter.cpp
        src/Fps_counter.cpp
        src/Light_component.cpp
        src/Light_provider.cpp
        src/Material.cpp
        src/Material_manager.cpp
        src/Math_constants.cpp
        src/Mesh_data.cpp
        src/Mesh_data_component.cpp
        src/Mesh_utils.cpp
        src/Mesh_vertex_layout.cpp
        src/Mikk_tspace_triangle_mesh_interface.cpp
        src/Morph_weights_component.cpp
        src/PBR_material.cpp
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
        "src/Collision.cpp"
        "src/Statics.cpp"
        "src/OeCore.cpp"
        "ThirdParty/MikktSpace/mikktspace.c"
        "ThirdParty/imgui/misc/cpp/imgui_stdlib.cpp")

# CPP17
target_compile_features(OeCore PUBLIC cxx_std_17)
if (MSVC)
    # There is a bug in visual studio that prevents intellisense from realizing
    # /std:c++17 is on the command line if you only use target_compile_features(cxx_std_17)
    target_compile_options(${PROJECT_NAME} PUBLIC "/std:c++17")
endif()

add_library(Oe::Core ALIAS ${PROJECT_NAME})

oe_target_generate_export_header(${PROJECT_NAME})

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
        include/MikktSpace
        include/imgui/misc/cpp
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

# Guidelines Support Library (gsl::)
find_package(Microsoft.GSL)
target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft.GSL::Microsoft.GSL)

# C++20 Ranges
find_package(range-v3)
target_link_libraries(${PROJECT_NAME} PUBLIC range-v3::range-v3)

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

# Precompiled Headers - Only for C++ as WRL cannot be included into C.
target_precompile_headers(${PROJECT_NAME}
        PUBLIC
        $<$<COMPILE_LANGUAGE:CXX>:<OeCore/OeCore.h$<ANGLE-R>>
        PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:
        <OeCore/OeCore.h$<ANGLE-R>
        include/OeCore/WindowsDefines.h
        <vectormath.hpp$<ANGLE-R>
        <g3log/g3log.hpp$<ANGLE-R>
        <algorithm$<ANGLE-R>
        <exception$<ANGLE-R>
        <memory$<ANGLE-R>
        <stdexcept$<ANGLE-R>
        <cstdio$<ANGLE-R>
        <json.hpp$<ANGLE-R>>
        )
set_target_properties(${PROJECT_NAME} PROPERTIES
        SKIP_PRECOMPILE_HEADERS
        "ThirdParty/MikktSpace/mikktspace.c;ThirdParty/imgui/misc/cpp/imgui_stdlib.cpp")

# Make sure to include PDB files when installing in debug mode
oe_target_install_pdb(${PROJECT_NAME})

#####
# Enums
#####
oe_target_generate_enum(${PROJECT_NAME}
        SOURCE "${PROJECT_SOURCE_DIR}/codegen/Renderer_enums.json"
        BYPRODUCTS include/${PROJECT_NAME}/Renderer_enums.h
)