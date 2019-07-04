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
    python -m pip --version install pytest
    call :oe_verify_errorlevel "install python PIP" || goto:eof
)

set "OE_ROOT=%cd%"

if not defined VSINSTALLDIR (
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

set OE_CMAKE_EXE=%DevEnvDir%COMMONEXTENSIONS\MICROSOFT\CMAKE\CMake\bin\cmake.exe
set OE_NINJA_EXE=%DevEnvDir%COMMONEXTENSIONS\MICROSOFT\CMAKE\Ninja\ninja.exe
set OE_INSTALL_PREFIX=%OE_ROOT%\bin\x64

rem ******************************
rem Build Steps
rem ******************************

rem pybind11
call :cmake_ninja_install "%OE_ROOT%\thirdparty\pybind11", pybind11

rem g3log
call :cmake_ninja_install "%OE_ROOT%\thirdparty\g3log", g3log

rem MikktSpace and tinygltf are build using our own CMakeLists file.
call :cmake_ninja_install "%OE_ROOT%\thirdparty", thirdparty

rem Google Test
call :cmake_msvc_shared "%OE_ROOT%\thirdparty\googletest\googletest", gtest

rem Google Mock
call :cmake_msvc_shared "%OE_ROOT%\thirdparty\googletest\googlemock", gmock

rem DirectXTK
cd %OE_ROOT%\thirdparty\DirectXTK

set "CM_LOGFILE=%OE_ROOT%\thirdparty\DirectXTK\bin\oe-build-log.txt"
echo DirectXTK build output: %CM_LOGFILE%

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
msbuild %OE_DXTK_SLN%.sln /p:Configuration=Debug /p:Platform="x64" > %CM_LOGFILE%
call :oe_verify_errorlevel "msbuild Debug" || goto:eof
msbuild %OE_DXTK_SLN%.sln /p:Configuration=Release /p:Platform="x64" >> %CM_LOGFILE%
call :oe_verify_errorlevel "msbuild Release" || goto:eof

rem ******************************
rem Hacky Steps
rem ******************************

rem For some reason, the g3logger cmake find_module needs these directories to exist. Even though they are empty.
IF NOT EXIST "%OE_INSTALL_PREFIX%\Debug\COMPONENT" md "%OE_INSTALL_PREFIX%\Debug\COMPONENT"
IF NOT EXIST "%OE_INSTALL_PREFIX%\Release\COMPONENT" md "%OE_INSTALL_PREFIX%\Release\COMPONENT"
IF NOT EXIST "%OE_INSTALL_PREFIX%\Debug\libraries" md "%OE_INSTALL_PREFIX%\Debug\libraries"
IF NOT EXIST "%OE_INSTALL_PREFIX%\Release\libraries" md "%OE_INSTALL_PREFIX%\Release\libraries"

goto:eof

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
call :oe_verify_errorlevel "cmake" || goto:eof

ninja install >> %CM_LOGFILE%
call :oe_verify_errorlevel "ninja install" || goto:eof

set OE_GENERATE_BUILD_CONFIG=Release
set "CM_NINJA_BUILD_PATH=%OE_ROOT%\thirdparty\cmake-%CM_TARGET_NAME%-ninjabuild-x64-!OE_GENERATE_BUILD_CONFIG!"
IF NOT EXIST "!CM_NINJA_BUILD_PATH!" md !CM_NINJA_BUILD_PATH!
cd !CM_NINJA_BUILD_PATH!
set "CM_LOGFILE=%CM_NINJA_BUILD_PATH%\oe-build-log.txt"
echo %CM_TARGET_NAME% %OE_GENERATE_BUILD_CONFIG% build output: %CM_LOGFILE%
IF EXIST "!CM_NINJA_BUILD_PATH!\CMakeCache.txt" del "!CM_NINJA_BUILD_PATH!\CMakeCache.txt"
"%OE_CMAKE_EXE%" -G "Ninja" -DCMAKE_CXX_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_MAKE_PROGRAM="%OE_NINJA_EXE%" -DCMAKE_INSTALL_PREFIX:PATH="%OE_INSTALL_PREFIX%/%OE_GENERATE_BUILD_CONFIG%" -DCMAKE_BUILD_TYPE="!OE_GENERATE_BUILD_CONFIG!" "%CM_NINJA_SOURCE_PATH%" > %CM_LOGFILE%
call :oe_verify_errorlevel "cmake" || goto:eof

ninja install >> %CM_LOGFILE%
call :oe_verify_errorlevel "ninja install" || goto:eof

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
call :oe_verify_errorlevel "cmake" || goto:eof

msbuild %CM_TARGET_NAME%.sln /p:Configuration=Debug /p:Platform="x64" >> %CM_LOGFILE%
call :oe_verify_errorlevel "msbuild Debug" || goto:eof

msbuild %CM_TARGET_NAME%.sln /p:Configuration=Release /p:Platform="x64" >> %CM_LOGFILE%
call :oe_verify_errorlevel "msbuild Release" || goto:eof

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