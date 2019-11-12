@echo off
setlocal enableextensions
setlocal EnableDelayedExpansion

rem ******************************
rem Load Environment
rem ******************************

rem Check that the correct python version is installed.
for /f "tokens=*" %%a in ('python -c "exec(""import platform\n(bits,linkage)=platform.architecture()\n(major,minor,patch)=platform.python_version_tuple()\nprint('Python',major,bits,'OK' if int(major)==3 and int(minor)>=6 else 'TOO_OLD')"")"') do @set "PYTHON_VERSION_OK=%%a"
for /f "tokens=*" %%a in ('python --version') do @set "PYTHON_VERSION=%%a"
if NOT "%PYTHON_VERSION_OK%" == "Python 3 64bit OK" (
    echo * detected major version/arch: %PYTHON_VERSION_OK%
    echo * detected minor version: %PYTHON_VERSION%
    echo * [[31mFailed[0m] Invalid python version. Must have 3.6 64 bit or greater.
    goto:eof
) else (
    echo * [[32mOK[0m] Python version: %PYTHON_VERSION%, %PYTHON_VERSION_OK%
)

rem check that pip is installed
python -m pip -V > NUL
call :oe_verify_errorlevel "check python PIP is installed" || goto:eof

rem install pytest if it's missing
python -m pip show pytest > NUL
if "%errorlevel%" == "1" (
    python -m pip install pytest > NUL
    call :oe_verify_errorlevel "python -m pip install pytest" || goto:eof
) else (
    rem this should always succeed, just using it for a nicely formatted OK message
    call :oe_verify_errorlevel "check pytest installed"
)

set "OE_ROOT=%cd%"

if not defined VSINSTALLDIR (
    powershell -command "Get-VSSetupInstance" > NUL
    rem If this failed, run the following in a regular powershell window:
    rem Install-Module VSSetup -Scope CurrentUser
    call :oe_verify_errorlevel "Detect Get-VSSetupInstance powershell utility" || goto:eof

    for /f "tokens=*" %%a in ('powershell -command "Get-VSSetupInstance | Select-VSSetupInstance -Require Microsoft.VisualStudio.Workload.NativeDesktop -Latest | Select-Object -ExpandProperty InstallationPath"') do @set "OE_VC_INSTALL_PATH=%%a"

    IF EXIST "!OE_VC_INSTALL_PATH!\VC\Auxiliary\Build\vcvars64.bat" (
        echo Using visual studio at: !OE_VC_INSTALL_PATH!
        call "!OE_VC_INSTALL_PATH!\VC\Auxiliary\Build\vcvars64.bat"
    ) ELSE (
        echo "Could not find vcvars64.bat. Is VSSetup powershell module, and visual studio community edition installed?"
        goto:eof
    )
)

if "%VSCMD_ARG_TGT_ARCH%"=="x86" (
    call "%VCINSTALLDIR%\Auxiliary\Build\vcvars64.bat"
)

if not "%VisualStudioVersion%" == "15.0" (
    echo "The-Forge doesn't support VS 2019 yet (blah!). Please run this from the VS 2017 Developer Command Prompt."
    goto:eof
) 

set "OE_CMAKE_EXE=%DevEnvDir%COMMONEXTENSIONS\MICROSOFT\CMAKE\CMake\bin\cmake.exe"
set "OE_NINJA_EXE=%DevEnvDir%COMMONEXTENSIONS\MICROSOFT\CMAKE\Ninja\ninja.exe"
set "OE_INSTALL_PREFIX=%OE_ROOT%\bin\x64"

rem ******************************
rem Clone third party repositories
rem ******************************

cd "%OE_ROOT%"
set "OE_THIRDPARTY_OUTPUT=%OE_ROOT%\thirdparty\update-thirdparty.log"
echo Updating thirdparty repositories: %OE_THIRDPARTY_OUTPUT%
if not exist "%OE_ROOT%\thirdparty\glTF-Sample-Models" (
    echo * [[33mThis will take 5 to 10 minutes, go walk the dog.[0m]
) else (
    echo * [[33mThis will take a few minutes if there are changes since your last run.[0m]
)
python Orangine\cmake\scripts\update-thirdparty.py 1> %OE_THIRDPARTY_OUTPUT% 2>&1
call :oe_verify_errorlevel "update-thirdparty.py" || goto:eof

rem ******************************
rem Build Steps
rem ******************************

rem pybind11
call :cmake_ninja_install "%OE_ROOT%\thirdparty\pybind11", pybind11 || goto:eof

rem g3log
call :cmake_ninja_install "%OE_ROOT%\thirdparty\g3log", g3log || goto:eof

rem MikktSpace and tinygltf are build using our own CMakeLists file.
call :cmake_ninja_install "%OE_ROOT%\thirdparty", thirdparty || goto:eof

rem Google Test
call :cmake_msvc_shared "%OE_ROOT%\thirdparty\googletest\googletest", gtest || goto:eof

rem Google Mock
call :cmake_msvc_shared "%OE_ROOT%\thirdparty\googletest\googlemock", gmock || goto:eof

rem DirectXTK
cd %OE_ROOT%\thirdparty\DirectXTK

md "%OE_ROOT%\thirdparty\DirectXTK\bin"
set "OE_DIRECTXTK_LOGFILE=%OE_ROOT%\thirdparty\build_DirectXTK.txt"
echo DirectXTK build output: %OE_DIRECTXTK_LOGFILE%

if "%VisualStudioVersion%" == "15.0" (
    set OE_DXTK_SLN=DirectXTK_Desktop_2017_Win10
) 
if "%VisualStudioVersion%" == "16.0" (
    set OE_DXTK_SLN=DirectXTK_Desktop_2019_Win10
)
if "%OE_DXTK_SLN%" == "" (
    echo * [[31mFailed[0m] Unsupported visual studio version: %VisualStudioVersion%
    goto:eof
)
msbuild %OE_DXTK_SLN%.sln /p:Configuration=Debug /p:Platform="x64" > %OE_DIRECTXTK_LOGFILE%
call :oe_verify_errorlevel "msbuild DirectXTK Debug" || goto:eof
msbuild %OE_DXTK_SLN%.sln /p:Configuration=Release /p:Platform="x64" >> %OE_DIRECTXTK_LOGFILE%
call :oe_verify_errorlevel "msbuild DirectXTK Release" || goto:eof

rem The-Forge
set "OE_THEFORGE_LOGFILE=%OE_ROOT%\thirdparty\build_The-Forge.txt"
echo The-Forge build output: %OE_THEFORGE_LOGFILE%

cd "%OE_ROOT%\thirdparty\The-Forge\Examples_3\Unit_Tests\PC Visual Studio 2017"
msbuild Unit_Tests.sln /p:Configuration=DebugDx /p:Platform="x64" -target:OS;Renderer\RendererDX12 > %OE_THEFORGE_LOGFILE%
call :oe_verify_errorlevel "msbuild The-Forge DebugDx" || goto:eof
msbuild Unit_Tests.sln /p:Configuration=ReleaseDx /p:Platform="x64" -target:OS;Renderer\RendererDX12 > %OE_THEFORGE_LOGFILE%
call :oe_verify_errorlevel "msbuild The-Forge ReleaseDx" || goto:eof

rem ******************************
rem End of main script.
rem ******************************

goto:eof

rem ******************************
rem Hacky Steps
rem ******************************

rem For some reason, the g3logger cmake find_module needs these directories to exist. Even though they are empty.
IF NOT EXIST "%OE_INSTALL_PREFIX%\Debug\COMPONENT" md "%OE_INSTALL_PREFIX%\Debug\COMPONENT"
IF NOT EXIST "%OE_INSTALL_PREFIX%\Release\COMPONENT" md "%OE_INSTALL_PREFIX%\Release\COMPONENT"
IF NOT EXIST "%OE_INSTALL_PREFIX%\Debug\libraries" md "%OE_INSTALL_PREFIX%\Debug\libraries"
IF NOT EXIST "%OE_INSTALL_PREFIX%\Release\libraries" md "%OE_INSTALL_PREFIX%\Release\libraries"

rem ******************************
rem Build Function Definitions
rem ******************************

:cmake_ninja_install
rem Takes 2 args; source path, build path, target name

set "CM_NINJA_SOURCE_PATH=%~1"
set "CM_TARGET_NAME=%~2"

set OE_GENERATE_BUILD_CONFIG=Debug
set "CM_NINJA_BUILD_PATH=%OE_ROOT%\thirdparty\cmake-%CM_TARGET_NAME%-ninjabuild-x64-!OE_GENERATE_BUILD_CONFIG!"
IF NOT EXIST "!CM_NINJA_BUILD_PATH!" md !CM_NINJA_BUILD_PATH!
cd "!CM_NINJA_BUILD_PATH!"
set "CM_LOGFILE=%CM_NINJA_BUILD_PATH%\oe-build-log.txt"
echo %CM_TARGET_NAME% %OE_GENERATE_BUILD_CONFIG% build output: %CM_LOGFILE%
IF EXIST "!CM_NINJA_BUILD_PATH!\CMakeCache.txt" del "!CM_NINJA_BUILD_PATH!\CMakeCache.txt"
"%OE_CMAKE_EXE%" -G "Ninja" -DCMAKE_CXX_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_MAKE_PROGRAM="%OE_NINJA_EXE%" -DCMAKE_DEBUG_POSTFIX=_d -DCMAKE_INSTALL_PREFIX:PATH="%OE_INSTALL_PREFIX%/%OE_GENERATE_BUILD_CONFIG%" -DCMAKE_BUILD_TYPE="!OE_GENERATE_BUILD_CONFIG!" "%CM_NINJA_SOURCE_PATH%" > %CM_LOGFILE%
call :oe_verify_errorlevel "cmake" || EXIT /B 1

ninja install >> %CM_LOGFILE%
call :oe_verify_errorlevel "ninja install" || EXIT /B 1

set OE_GENERATE_BUILD_CONFIG=Release
set "CM_NINJA_BUILD_PATH=%OE_ROOT%\thirdparty\cmake-%CM_TARGET_NAME%-ninjabuild-x64-!OE_GENERATE_BUILD_CONFIG!"
IF NOT EXIST "!CM_NINJA_BUILD_PATH!" md !CM_NINJA_BUILD_PATH!
cd !CM_NINJA_BUILD_PATH!
set "CM_LOGFILE=%CM_NINJA_BUILD_PATH%\oe-build-log.txt"
echo %CM_TARGET_NAME% %OE_GENERATE_BUILD_CONFIG% build output: %CM_LOGFILE%
IF EXIST "!CM_NINJA_BUILD_PATH!\CMakeCache.txt" del "!CM_NINJA_BUILD_PATH!\CMakeCache.txt"
"%OE_CMAKE_EXE%" -G "Ninja" -DCMAKE_CXX_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_MAKE_PROGRAM="%OE_NINJA_EXE%" -DCMAKE_INSTALL_PREFIX:PATH="%OE_INSTALL_PREFIX%/%OE_GENERATE_BUILD_CONFIG%" -DCMAKE_BUILD_TYPE="!OE_GENERATE_BUILD_CONFIG!" "%CM_NINJA_SOURCE_PATH%" > %CM_LOGFILE%
call :oe_verify_errorlevel "cmake" || EXIT /B 1

ninja install >> %CM_LOGFILE%
call :oe_verify_errorlevel "ninja install" || EXIT /B 1

EXIT /B 0

:cmake_msvc_shared
rem Takes 2 args; source path, build path, target name

set "CM_MSVC_SOURCE_PATH=%~1"
set "CM_TARGET_NAME=%~2"
set "CM_MSVC_BUILD_PATH=%OE_ROOT%\thirdparty\cmake-%CM_TARGET_NAME%-build-x64"

IF NOT EXIST "%CM_MSVC_BUILD_PATH%" (
    md "%CM_MSVC_BUILD_PATH%"
)
cd "%CM_MSVC_BUILD_PATH%"
IF EXIST "%CM_MSVC_BUILD_PATH%\CMakeCache.txt" del "%CM_MSVC_BUILD_PATH%\CMakeCache.txt"

if "%VisualStudioVersion%" == "15.0" (
    SET "OE_CMAKE_GENERATE=Visual Studio 15 2017"
)
if "%VisualStudioVersion%" == "16.0" (
    SET "OE_CMAKE_GENERATE=Visual Studio 16 2019"
)

set "CM_LOGFILE=%CM_MSVC_BUILD_PATH%\oe-build-log.txt"
echo %CM_TARGET_NAME% build output: %CM_LOGFILE%

"%OE_CMAKE_EXE%" -DBUILD_SHARED_LIBS=ON -G "%OE_CMAKE_GENERATE%" -A x64 "%CM_MSVC_SOURCE_PATH%" > %CM_LOGFILE%
call :oe_verify_errorlevel "cmake" || EXIT /B 1

msbuild %CM_TARGET_NAME%.sln /p:Configuration=Debug /p:Platform="x64" >> %CM_LOGFILE%
call :oe_verify_errorlevel "msbuild Debug" || EXIT /B 1

msbuild %CM_TARGET_NAME%.sln /p:Configuration=Release /p:Platform="x64" >> %CM_LOGFILE%
call :oe_verify_errorlevel "msbuild Release" || EXIT /B 1

EXIT /B 0

:oe_verify_errorlevel
rem Takes 1 arg; operation name
if not %errorlevel% == 0 ( 
    echo * [[31mFailed[0m] %~1 : errorlevel=%errorlevel%!
    echo Step failed, aborting.
    EXIT /B 1
) else (
    echo * [[32mOK[0m] %~1
)
EXIT /B 0