# Software Pre-requisites
To get started, you need to make sure the following software is installed on your system.

## Windows 10
You must be running Windows 10 build version 2004 (20H1) or higher. Run the `winver` program to view which build version you have. If you are running an earlier version of Windows, run Windows Update or [Download Windows 10 Disk Image](https://go.microsoft.com/fwlink/p/?LinkId=821363)

## Visual Studio
Visual Studio must be installed in order to satisfy library dependencies and get the windows SDK.

An up to date Visual Studio 2019 

Visual Studio 2019 must be installed. When installing, ensure you select the following options (in addition to whatever defaults are selected):

- Desktop Development with Visual C++
  - C++ CMake tools for Windows (see https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-160)
- Game Development with C++
  - Windows 10 SDK (10.0.19041.0). Why?:
    - 18362 is min version required to fix this bug: https://bugs.chromium.org/p/chromium/issues/detail?id=969698
    - 19041 is min version supported by DirectX Helper Library (d3dx12.h)    
  - Test Adapter for Google Test (If you want to use the visual studio gtest UI)
- Python Development (if you want to use mixed mode python debugging in the editor, which you should want! See: https://docs.microsoft.com/en-us/visualstudio/python/debugging-mixed-mode-c-cpp-python-in-visual-studio?view=vs-2019)
  - Python native development tools

## Visual studio find module
(This is a microsoft module, see here: https://github.com/microsoft/vssetup.powershell)

In a regular powershell window, run the following command (and agree to the prompts to install PowerShellGet, and untrusted PSGallery warning):

```
Install-Module VSSetup -Scope CurrentUser
```

Then, enable powershell execution:

```
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

## Python 3.7
Orangine uses python for both helper scripts in the build process, and also to link against for purposes of embedding python scripting functionality to the engine.

Python 3 (both 32 and 64 bit) must be installed and on the system path.

Download 64bit [Python 3.7.6 installer](https://www.python.org/ftp/python/3.7.6/python-3.7.6-amd64.exe) from Python website. 

- You'll probably want to select the option to add python 64bit to the system PATH

Download 32bit [Python 3.7.6 installer](https://www.python.org/ftp/python/3.7.6/python-3.7.6.exe) from Python website.

(Note on python version: Python 3.7.3 doesn't work if you install debug binaries, as there is a bug preventing creation of venv's with debug symbols installed.)

## Conan
[Conan](https://conan.io/) is a widely used package manager for C++. This project has embedded conan calls into its CMakeLists files to pull open source dependencies.

Install Conan from the comman line using Python.

```
pip install conan
```

# Getting the source code
## Git SSH Keys
Test GIT connectivity using git bash:

```
ssh git@github.com
```

If either of those fail, add your ssh keys to the website using by [following these instructions](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent).

## Clone the Orangine Repo
clone the repository to somewhere convenient on your hard drive:

```
git clone git@github.com:leweaver/orangine.git c:\repos\orangine
```

# Build Orangine
The project is defined using CMake. The main project file is `<git-repo>\Orangine\CMakeLists.txt`, which can be opened by your favourite CMake compatible IDE (CLion, Visual Studio, VS Code, etc.)

This project makes use of [CMakePresets](https://www.jetbrains.com/help/clion/2021.2/cmake-presets.html) to do the heavy lifting of configuration. It is set to use Ninja for builds by default, though you can of course use whatever you like.

## CLion
(Instructions written against CLion 2021.2.3)

When you launch CLion, open an existing project:`<git-repo>\Orangine\CMakeLists.txt`

You will need to create 2 visual studio 2019 toolchains within CLion, which are used for building. The name of these toolchains is **very important**; they are referenced by name from the CMakePresets files.

In File -> Settings -> Build, Execution, Deployment -> Toolchains,

* 64bit Toolchain
  * Picture: (docs/clion-toolchain.png)
  * Name: Visual Studio 2019 AMD64
  * Environment: C:\Program Files (x86)\Microsoft Visual Studio\2019\Community
  * Architecture: AMD64 (*Note: x86_AMD64 will NOT work*)

* 32bit Toolchain
  * Picture: (docs/clion-toolchain-x86.png)
  * Name: Visual Studio 2019 x86
  * Environment: C:\Program Files (x86)\Microsoft Visual Studio\2019\Community
  * Architecture: x86

Then; in File -> Settings -> Build, Execution, Deployment -> CMake

* Enable the "dev preset" profile by clicking on it, and checking the "Enable Profile" checkbox.

You should now be able to build the "ViewerApp" target. (Select it from the dropdown next to the play icon at the top right of the IDE)

You can ignore the following error message in the CMake console, it is just Clion freaking out and has no impact on the build: 

```
Problems were encountered while collecting compiler information:
	C:\Users\hotma\AppData\Local\Temp\compiler-file13904199550438012870: fatal error C1083: Cannot open include file: 'pch.h': No such file or directory
```

## Visual Studio

1. Launch Visual Studio
1. On the startup page, click the tiny blue link to 'Continue without Code'
1. File -> Open -> CMake -> `<git-repo>\Orangine\CMakeLists.txt`

From here, you can select one of two startup projects in the "Select Startup Item" dropdown:

- __ViewerApp.exe (ViewerApp\ViewerApp.exe)__
  - This will build and run the sample executable.
- __ViewerApp.exe (Install)__
  - This will build the engine lib's and sample exe's, placing them in a directory for import into other projects.

The game files will be built/installed to the following location:

`%LOCALAPPDATA%/Orangine`

Some tips on Visual Studio with CMake: https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=vs-2019

## Option 2: Your IDE of choice

I've tested the CMake support in CLion, VSCode, Visual Studio 2017/2019 and all seem to work.

## Option 3: On the command line
You can also run CMake commands yourself on the command line.

If you want to use ninja to build, `<git-repo>\Orangine\oe-built.bat` wraps CMake and ninja commands:

- `oe-built.bat Configure` _Uses CMake to creates ninja build definition files. You will only need to run this once_
- `oe-built.bat Build Debug` _Calls_ __ninja build__ _in the cmake cache directory. Also supports `Release` as the second arg_
- `oe-built.bat Install Debug` _Uses ninja to build and place the engine libraries to the root `bin` directory, for consumption by other repositories_

You can manually run any other ninja command: list all of the available ninja commands:`cd cmake-ninjabuild-x64-Debug`

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

## Exceptions (C++)
C++ Exceptions are (currently) enabled, but should only be used when something has seriously gone wrong (such as a missing asset file). It may be possible for the program to continue; however it will need to recover and work around the issue in some way.

Never raise or catch exceptions to direct program flow in normal use cases (in other words, do not program C++ code as if it were Python, where it is actually Pythonic to use exceptions for this purpose.)

You must throw all exceptions using the OE_THROW() macro. In debug builds the OE_THROW macro will cause a fatal program exit and print out the stack, which helps enforce the above rules.

## Forward Declarations
Don't use for model classes in 'Manager' class header files (ie, for the things that the manager class managers)
Do use when referencing manager classes in header files

## Pointers, References, Smart Pointers
Using Core CPP guidelines; but it is worth copying them here on this point:

- Don’t pass a smart pointer as a function parameter unless you want to use or manipulate the smart pointer itself, such as to share or transfer ownership.
- Express that a function will store and share ownership of a heap object using a by-value shared_ptr parameter.
- Use a non-const shared_ptr& parameter only to modify the shared_ptr. Use a const shared_ptr& as a parameter only if you’re not sure whether or not you’ll take a copy and share ownership; otherwise use widget* instead (or if not nullable, a widget&).

## Naming
[https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#nl10-prefer-underscore_style-names](NL.10) Prefer underscore_case for names
[https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#nl17-use-kr-derived-layout](NL.17) use KR derived layout

```
namespace: lower_underscored
classes, structs, enums: My_class_name
template parameters: TMy_class_name

private field members: _lowerCamelCase
public field members: lowerCamelCase
```


# Apendices

## Optional: Linking against python's debug binaries
Prefix: I really don't recommend doing this. But it is possible.

This isn't required, unless you want to debug the actual Python source code. Even then, I'd suggest
making a small unit test and debugging the outside of Orangine, using python_d.exe.

Linking against debug python libraries in Orangine makes some things really hard...
you must also build the pip dependencies with debug python. Most support this; but not all.

If you want to forge ahead, it DOES WORK. All, with the exception of remote debugging python code.

1. When installing Python, both 32 and 64 bit, choose _advanced options_ and enable:

- Download debug binaries

> Make sure it is installed to: `%LOCALAPPDATA%\Programs\Python\Python37[_32]\`

2. When generating the Orangine CMake project, set the following CMake Cache variable:

```
-DOeScripting_PYTHON_DEBUG=ON
```

## (Outdated) Running unit tests
> Unit tests haven't yet been updated to use CMake; so the following may be an OK guide but probably won't work

First, make sure tests are set to run in x64.

1. Test -> Test Settings -> Select Test Settings File
1. Choose `<git-repo>\Engine-Test\x64.runsettings`

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