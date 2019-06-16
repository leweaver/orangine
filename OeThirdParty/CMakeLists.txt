project(OeThirdParty VERSION 1.0
        DESCRIPTION "Orangine ThirdParty Library Wrappers"
        LANGUAGES CXX C)

# ImGui library
add_library(OeImgui STATIC
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/examples/imgui_impl_dx11.h"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/examples/imgui_impl_win32.h"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/imconfig.h"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/imgui.h"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/imgui_internal.h"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/imstb_rectpack.h"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/imstb_truetype.h"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/misc/cpp/imgui_stdlib.h"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/examples/imgui_impl_dx11.cpp"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/examples/imgui_impl_win32.cpp"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/imgui.cpp"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/imgui_demo.cpp"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/imgui_draw.cpp"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/imgui_widgets.cpp"
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui/misc/cpp/imgui_stdlib.cpp"
)
add_library(Oe::Imgui ALIAS OeImgui)
target_include_directories(OeImgui
        PUBLIC
        "${Orangine_SOURCE_DIR}/thirdparty/imgui"
        PRIVATE
        "${Orangine_SOURCE_DIR}/thirdparty/imgui/imgui"
        )
target_link_libraries(OeImgui PUBLIC OeCompilerFlags)

# MikktSpace library
add_library(MikktSpace STATIC
        "${Orangine_SOURCE_DIR}/thirdparty/MikktSpace/include/MikktSpace/mikktspace.h"
        "${Orangine_SOURCE_DIR}/thirdparty/MikktSpace/src/mikktspace.c"
        )
add_library(Oe::MikktSpace ALIAS MikktSpace)
target_include_directories(MikktSpace
        PUBLIC
        "${Orangine_SOURCE_DIR}/thirdparty/MikktSpace/include"
        PRIVATE
        "${Orangine_SOURCE_DIR}/thirdparty/MikktSpace/include/MikktSpace"
        )
target_link_libraries(OeImgui PUBLIC OeCompilerFlags)

