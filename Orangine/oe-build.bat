@echo off
setlocal enableextensions
setlocal EnableDelayedExpansion

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
    echo     CONFIG    = debug, release
    GOTO:eof
)

set "OE_ROOT=%cd%"

rem This line MUST be run at root scope. otherwise weirdness.
FOR /f "tokens=1-2* delims= " %%a in ( 'reg query "HKLM\Software\WOW6432Node\Microsoft\VisualStudio\SxS\VS7" /V 15.0 ^| find "REG_SZ" ' ) do set OE_VS15DIR=%%c

if not defined VSINSTALLDIR (
    IF EXIST "%OE_VS15DIR%VC\Auxiliary\Build\vcvars64.bat" (
        call "%OE_VS15DIR%VC\Auxiliary\Build\vcvars64.bat"
    ) ELSE (
        echo "Could not find vcvars64.bat. Is visual studio 2017 community edition installed?"
        goto:eof
    )
)

if not "%VSCMD_ARG_TGT_ARCH%"=="x64" (
    call "%VCINSTALLDIR%\Auxiliary\Build\vcvars64.bat"
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

echo OE_BUILD_DIR=%OE_BUILD_DIR%

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

set OE_CONFIGURE_BUILD_CONFIG=Release
set OE_BUILD_DIR=%OE_ROOT%\cmake-ninjabuild-x64-!OE_CONFIGURE_BUILD_CONFIG!
IF NOT EXIST "!OE_BUILD_DIR!" md !OE_BUILD_DIR!
cd !OE_BUILD_DIR!
"%OE_CMAKE_EXE%" -G "Ninja" -DCMAKE_CXX_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_C_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_MAKE_PROGRAM="%OE_NINJA_EXE%" -DCMAKE_INSTALL_PREFIX:PATH="%OE_ROOT%\..\bin\x64\!OE_CONFIGURE_BUILD_CONFIG!" -DCMAKE_BUILD_TYPE="!OE_CONFIGURE_BUILD_CONFIG!" "%OE_ROOT%"
endlocal

EXIT /B 0