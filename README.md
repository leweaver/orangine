# Software Pre-requisites

## Windows 10
You must be running Windows 10 build version 1703 (Creators Update) or higher. Run the `winver` program to view which build version you have. If you are running an earlier version of Windows, run Windows Update or [Download Windows 10 Disk Image](https://go.microsoft.com/fwlink/p/?LinkId=821363)

## Visual Studio
Visual Studio must be installed in order to satisfy library dependencies and get the windows SDK.

Visual Studio 2017 15.7.5 is the minimum required. (I've tested with 2019 and it also works.)

When installing, ensure you select the following options (in addition to whatever defaults are selected):

- Desktop Development with Visual C++
  - Windows 10 SDK (10.0.17134.0)
  - Test Adapter for Google Test (If you want to use the visual studio gtest UI)

## Git
If you are reading this you probably already have GIT installed. If not, [follow these instructions](https://confluence.atlassian.com/get-started-with-bitbucket/install-and-set-up-git-860009658.html)

# First time build

## Get the code & dependencies from GIT
1. Clone the repository
1. Download thirdparty dependencies: In the root repository directory, run: 

```
git submodule update --init --recursive
```

_Note_, to update the submodules to the latest version pointed to by the repository, run the following command:
```
git pull --recurse-submodules
git submodule update --recursive --remote
```

[More submodule tips](https://gist.github.com/gitaarik/8735255)

## Build dependencies
Some of the third party libraries need to be built first using helper script `.\create-thirdparty-projects.bat`:

- g3log
- googletest
- googlemock

## CMake
I've set the project up to use CMake, which is a build file generator. If you've not used CMake before, I recommend you simply open the main CMake file (`Orangine/CMakeLists.txt`) in visual studio (see: https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=vs-2019)

I've tested the CMake support in CLion, VSCode, Visual Studio 2017/2019 and all seem to work. (My IDE of choice is VS 2019)

You can also use a wrapper utility `Orangine/oe-built.bat` to compile without an IDE:

- `oe-built.bat Generate` _Uses CMake to creates ninja build definition files. You will only need to run this once_
- `oe-built.bat Build Debug` _Uses ninja to build the libraries and samples. Also supports `Release` as the second arg_
- `oe-built.bat Install Debug` _Uses ninja to build and place the engine libraries to the root `bin` directory, for consumption by other repositories_

# Overall Architecture
Here is a quick non-exhaustive overview of how the main object instances are instantiated. Each item creates and owns the items below it in the tree. Each item below is the name of a class, which is located in a CPP/H file of the same name. 

- oe::Main
  - DX::DeviceResources
  - oe::Game
    - oe::Scene
      - oe::Entity_graph_loader_gltf
      - oe::IManager_base derived classes:
        - oe::Entity_render_manager
        - oe::Scene_graph_manager
            - oe::Entity
            - oe::Entity_filter
        - oe::Render_step_manager
            - oe::Render_step
        - ...

# (Outdated) Running unit tests
> Unit tests haven't yet been updated to use CMake; so the following may be an OK guide but probably won't work

First, make sure tests are set to run in x64.

1. Test -> Test Settings -> Select Test Settings File 
1. Choose `.\Engine-Test\x64.runsettings`

Try one of these two methods... stick to the one that works for you!

## Unit Test Sessions
1. Build the solution
1. Right click on the `Engine-Test` project, and select `Run Tests`

## Test Explorer
> Note - running the tests in visual studio can be a real pain. The test explorer sometimes just refuses to find them.

Tests are executed via the `Test Explorer` window in visual studio. Projects that contain tests are suffixed with `-Test` and compile to an x64 executable. To run them;

1. Click the Run All button in the test explorer window. 
    - If not visible, Test -> Windows -> Test Explorer

> NOTE: Don't try to run individual tests; this seems to break the Test Explorer window. If in doubt, `Run All` makes things work again :) 

> Tip: Optionally you can unload the `Prototype` solution to make runs faster, since it doesn't contain any tests.



# Maths Conventions
Matrix conventions are as in SimpleMath: Right Handed, Row Major.

Some notes on libraries that are in use:

- __DirectXMath__ offers both left-handed and right-handed versions of matrix functions with 'handedness', but always assumes a row-major format.
- __SimpleMath__ as with DirectXMath uses row-major ordering for matrices.
- __HLSL__     defaults to using column-major ordering as this makes for slightly more efficient shader matrix multiplies. Therefore, when a Matrix is going to be copied into a HLSL constant buffer, it is usually transposed as part of updating the constant buffer.  

# Renderer 
## Normals and Tangents
Normals are expected to be in tangent space. Further, the tangents on meshes must be generated using the MikkiT algorithm.

Tangents in art assets must be generated using the MikkiT algorithm (see [Simulation of Wrinkled Surfaces Revisited by Morten S. Mikkelsen](http://image.diku.dk/projects/media/morten.mikkelsen.08.pdf).)

# Coding Style Guide
Using [Cpp Core Guidelines](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md) with some additions/clarifications:

## Forward Declarations
Don't use for model classes in 'Manager' class header files (ie, for the things that the manager class managers)
Do use when referencing manager classes in header files

## Pointers, References, SmartPointers
Function Arguments:
    [https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rr-smartptrparam](R.30)
    Prefer references to objects, but take a pointer if it can be null.
    Only take in a smart/unique pointer when lifetime/ownership needs to be managed:
        [https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rr-smartptrparam](R.36)
        ```
        void share(shared_ptr<widget>);            // share -- "will" retain refcount
        void reseat(shared_ptr<widget>&);          // "might" reseat ptr
        void may_share(const shared_ptr<widget>&); // "might" retain refcount
        ```

## Naming
[https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#nl10-prefer-underscore_style-names](NL.10) Prefer    underscore_case for names
[https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#nl17-use-kr-derived-layout](NL.17) use KR derived layout

namespace: lower_underscored
classes, structs, enums: My_class_name
template parameters: TMy_class_name

private field members: _lowerCamelCase
public field members: lowerCamelCase

## Install Python 3.7
Download [Python 3.7.3 installer](https://www.python.org/ftp/python/3.7.3/python-3.7.3-amd64.exe) from Python website.

In the installer, choose _advanced options_ and enable:

- Download debug binaries

then make sure it is installed to the LOCALAPPDATA folder:

```
C:\Users\<your-username>\AppData\Local\Programs\Python\Python37\
```

