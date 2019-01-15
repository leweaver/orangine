# 3rd Party Library Considerations:
[Crazy Eddie's GUI System v0.8.7](http://cegui.org.uk/) - Lots of dependencies, which use CMake to build. The DirectX11 renderer (in the stable branch, anyway) uses the effect framework, which is deprecated. One forum post suggested that the _default) hg branch has fixed this; but shouldn't be used for prod code. Junking this, as it hasn't had a release in 2 years, and is remarkably tricky to build.

[imgui](https://github.com/ocornut/imgui/) - This is used by Oculus, so probably is worth a look at least!
- https://www.youtube.com/watch?v=LSRJ1jZq90k
- https://www.youtube.com/watch?v=Btx_tujnyB4
- https://www.youtube.com/watch?v=o5sJClp0HDY

[nuklear](https://github.com/vurtun/nuklear/) - Another immediate mode GUI. Might be slower on the updates, as maintainer no [longer has time](https://github.com/vurtun/nuklear/issues/693#issuecomment-396060773)?





# ARCHIVE
Notes when evaluating CEGUI

## Warning signs:
Uses D3DX11Effects

## Instructions - CEGUI Deps
From: http://static.cegui.org.uk/docs/0.8.7/building_deps.html#building_deps_intro

1. Download and install [CMake Windows win64-x64 Installer](https://cmake.org/download/)
    - Make sure to add CMake to system path, when prompted.
1. [Download dependencies](https://sourceforge.net/projects/crayzedsgui/files/CEGUI%20Mk-2%20Dependencies/0.8.x/cegui-deps-0.8.x-src.zip/download) and extract to somewhere convenient, such as c:\repos
    - This should extract a single folder, like so: `c:\repos\cegui-cegui-dependencies-0ecdf3a9e49b`
1. In a regular command prompt,
    ```
    set CEGUI-DEPS=c:\repos\cegui-cegui-dependencies-0ecdf3a9e49b
    setx CEGUI-DEPS %CEGUI-DEPS%
    cd %CEGUI-DEPS%
    mkdir build
    cd build
    cmake-gui
    ```
1. Set some environment variables:
    - DIRECTXSDK_H_PATH="C:\Program Files (x86)\Windows Kits\10\Include\10.0.16299.0\um\"
    - DIRECTXSDK_MAX_D3D
1. In the window that appears, set the following:
    - Where is the source code: (click browse, then paste: `%CEGUI-DEPS%`)
    - Where to build the binaries: (click browse, then paste: `%CEGUI-DEPS%/build`)
    - Click Configure, and set the following options before hitting OK:
        - Specify the generator for this project: `Visual Studio 15 2017 Win64`
        - Use default native compilers
    - In the white console box at the bottom, read every line of text and make sure there are no errors!
    - Now, you should see some options with a red background. Make sure the following things are ticked:
        - CEGUI_BUILD_EXPAT
        - CEGUI_BUILD_FREETYPE2
        - CEGUI_BUILD_GLEW
        - CEGUI_BUILD_GLFW
        - CEGUI_BUILD_GLM
        - CEGUI_BUILD_PCRE
        - CEGUI_BUILD_SILLY
