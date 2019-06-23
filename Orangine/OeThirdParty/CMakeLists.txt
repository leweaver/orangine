project(OeThirdParty VERSION 1.0
        DESCRIPTION "Orangine ThirdParty Library Wrappers"
        LANGUAGES CXX C)

set(OE_THIRDPARTY_DIR "${Orangine_SOURCE_DIR}/../thirdparty")

# ImGui library
add_library(OeImgui STATIC
        "${OE_THIRDPARTY_DIR}/imgui/imgui/examples/imgui_impl_dx11.h"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/examples/imgui_impl_win32.h"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/imconfig.h"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/imgui.h"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/imgui_internal.h"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/imstb_rectpack.h"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/imstb_truetype.h"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/misc/cpp/imgui_stdlib.h"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/examples/imgui_impl_dx11.cpp"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/examples/imgui_impl_win32.cpp"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/imgui.cpp"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/imgui_demo.cpp"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/imgui_draw.cpp"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/imgui_widgets.cpp"
        "${OE_THIRDPARTY_DIR}/imgui/imgui/misc/cpp/imgui_stdlib.cpp"
)
add_library(Oe::Imgui ALIAS OeImgui)
target_include_directories(OeImgui
        PUBLIC
        "${OE_THIRDPARTY_DIR}/imgui"
        PRIVATE
        "${OE_THIRDPARTY_DIR}/imgui/imgui"
        )
target_link_libraries(OeImgui PUBLIC OeCompilerFlags)

# MikktSpace library
add_library(MikktSpace STATIC
        "${OE_THIRDPARTY_DIR}/MikktSpace/include/MikktSpace/mikktspace.h"
        "${OE_THIRDPARTY_DIR}/MikktSpace/src/mikktspace.c"
        )
add_library(Oe::MikktSpace ALIAS MikktSpace)
target_include_directories(MikktSpace
        PUBLIC
        "${OE_THIRDPARTY_DIR}/MikktSpace/include"
        PRIVATE
        "${OE_THIRDPARTY_DIR}/MikktSpace/include/MikktSpace"
        )
target_link_libraries(OeImgui PUBLIC OeCompilerFlags)

