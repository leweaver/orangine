@echo off
setlocal enableextensions
setlocal EnableDelayedExpansion

set OE_ARCHITECTURE=x64

echo Building %cd%

set OE_REQUIRES_ARG2=0
set OE_VALID_INPUT=0

set OE_ACTION_CONFIGURE=0
set OE_ACTION_CLEAN=0
set OE_ACTION_BUILD=0
set OE_ACTION_INSTALL=0

if "%1"=="generate" (
    set OE_ACTION_CONFIGURE=1
    set OE_VALID_INPUT=1
)
if "%1"=="clean" (
    set OE_ACTION_CLEAN=1
    set OE_REQUIRES_ARG2=1
)
if "%1"=="build" (
    set OE_ACTION_BUILD=1
    set OE_REQUIRES_ARG2=1
)
if "%1"=="install" (
    set OE_ACTION_INSTALL=1
    set OE_REQUIRES_ARG2=1
)

set "OE_BUILD_CONFIG="
if "%OE_REQUIRES_ARG2%" EQU "1" (
    if "%2"=="debug" set OE_BUILD_CONFIG=%2
    if "%2"=="release" set OE_BUILD_CONFIG=%2
    if "%2"=="relwithdebinfo" set OE_BUILD_CONFIG=%2
    if NOT "!OE_BUILD_CONFIG!" EQU "" set OE_VALID_INPUT=1
)

if "!OE_VALID_INPUT!" EQU "0" (
    echo.
    echo Usage: %0 BUILDTYPE [CONFIG]
    echo.
    echo   Where:
    echo     BUILDTYPE = generate, clean, build or install
    echo.
    echo   For Clean, Build or Install, provide:
    echo     CONFIG    = debug, relwithdebinfo, release
    GOTO:eof
)

set "OE_ROOT=%cd%"

if not defined VSINSTALLDIR (
    powershell -command "Get-VSSetupInstance" > NUL
    rem If this failed, run the following in a regular powershell window:
    rem Install-Module VSSetup -Scope CurrentUser
    if not %errorlevel% == 0 ( 
        "Could not find Get-VSSetupInstance powershell utility"
        goto:eof
    )

    for /f "tokens=*" %%a in ('powershell -command "Get-VSSetupInstance | Select-VSSetupInstance -Require Microsoft.VisualStudio.Workload.NativeDesktop -Latest | Select-Object -ExpandProperty InstallationPath"') do @set "OE_VC_INSTALL_PATH=%%a"

    IF EXIST "!OE_VC_INSTALL_PATH!\VC\Auxiliary\Build\vcvars64.bat" (
        echo Using visual studio at: !OE_VC_INSTALL_PATH!
        call "!OE_VC_INSTALL_PATH!\VC\Auxiliary\Build\vcvars64.bat"
    ) ELSE (
        echo "Could not find vcvars64.bat. Is VSSetup powershell module, and visual studio community edition installed?"
        goto:eof
    )
)

if not "%OE_ARCHITECTURE%"=="%VSCMD_ARG_TGT_ARCH%" (
    if "%OE_ARCHITECTURE%"=="x64" (
        call "%VCINSTALLDIR%Auxiliary\Build\vcvars64.bat"
    ) else (
        call "%VCINSTALLDIR%Auxiliary\Build\vcvars32.bat"
    )
)

set OE_CMAKE_EXE=%DevEnvDir%COMMONEXTENSIONS\MICROSOFT\CMAKE\CMake\bin\cmake.exe
set OE_NINJA_EXE=%DevEnvDir%COMMONEXTENSIONS\MICROSOFT\CMAKE\Ninja\ninja.exe

if "!OE_ACTION_CONFIGURE!" == "1" (
    echo Task: Generate
    
    call :cmake_generate
)

if NOT "!OE_BUILD_CONFIG!" EQU "" (
    set OE_BUILD_DIR=%OE_ROOT%\cmake-ninjabuild-x64-!OE_BUILD_CONFIG!

    IF NOT EXIST "!OE_BUILD_DIR!" (
        echo CMake cache does not exist, generating
        call :cmake_generate
    )
    cd !OE_BUILD_DIR!
)

if "!OE_ACTION_CLEAN!" == "1" (
    echo Task: Clean !OE_BUILD_CONFIG!
    ninja clean
)
if "!OE_ACTION_BUILD!" == "1" (
    echo Task: Build !OE_BUILD_CONFIG!
    ninja
)
if "!OE_ACTION_INSTALL!" == "1" (
    echo Task: Installing !OE_BUILD_CONFIG!
    ninja install
)

EXIT /B 0


REM Helper to generate cmake caches
:cmake_generate

setlocal
set OE_CONFIGURE_BUILD_CONFIG=Debug
set OE_BUILD_DIR=%OE_ROOT%\cmake-ninjabuild-x64-!OE_CONFIGURE_BUILD_CONFIG!
IF NOT EXIST "!OE_BUILD_DIR!" md !OE_BUILD_DIR!
cd !OE_BUILD_DIR!
"%OE_CMAKE_EXE%" -G "Ninja" -DCMAKE_CXX_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_C_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_MAKE_PROGRAM="%OE_NINJA_EXE%" -DCMAKE_DEBUG_POSTFIX=_d -DCMAKE_INSTALL_PREFIX:PATH="%OE_ROOT%\..\bin\x64\!OE_CONFIGURE_BUILD_CONFIG!" -DCMAKE_BUILD_TYPE="!OE_CONFIGURE_BUILD_CONFIG!" "%OE_ROOT%"

set OE_CONFIGURE_BUILD_CONFIG=RelWithDebInfo
set OE_BUILD_DIR=%OE_ROOT%\cmake-ninjabuild-x64-!OE_CONFIGURE_BUILD_CONFIG!
IF NOT EXIST "!OE_BUILD_DIR!" md !OE_BUILD_DIR!
cd !OE_BUILD_DIR!
"%OE_CMAKE_EXE%" -G "Ninja" -DCMAKE_CXX_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_C_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_MAKE_PROGRAM="%OE_NINJA_EXE%" -DCMAKE_INSTALL_PREFIX:PATH="%OE_ROOT%\..\bin\x64\!OE_CONFIGURE_BUILD_CONFIG!" -DCMAKE_BUILD_TYPE="!OE_CONFIGURE_BUILD_CONFIG!" "%OE_ROOT%"

set OE_CONFIGURE_BUILD_CONFIG=Release
set OE_BUILD_DIR=%OE_ROOT%\cmake-ninjabuild-x64-!OE_CONFIGURE_BUILD_CONFIG!
IF NOT EXIST "!OE_BUILD_DIR!" md !OE_BUILD_DIR!
cd !OE_BUILD_DIR!
"%OE_CMAKE_EXE%" -G "Ninja" -DCMAKE_CXX_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_C_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_MAKE_PROGRAM="%OE_NINJA_EXE%" -DCMAKE_INSTALL_PREFIX:PATH="%OE_ROOT%\..\bin\x64\!OE_CONFIGURE_BUILD_CONFIG!" -DCMAKE_BUILD_TYPE="!OE_CONFIGURE_BUILD_CONFIG!" "%OE_ROOT%"
endlocal

EXIT /B 0