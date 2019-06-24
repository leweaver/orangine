@echo off
setlocal enableextensions

set "OE_ROOT=%cd%"

rem This line MUST be run at root scope. otherwise weirdness.
FOR /f "tokens=1-2* delims= " %%a in ( 'reg query "HKLM\Software\WOW6432Node\Microsoft\VisualStudio\SxS\VS7" /V 15.0 ^| find "REG_SZ" ' ) do set OE_VS15DIR=%%c

if not defined VSINSTALLDIR (
    echo "OE_VS15DIR=%OE_VS15DIR%"
    IF EXIST "%OE_VS15DIR%VC\Auxiliary\Build\vcvarsall.bat" (
        call "%OE_VS15DIR%VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
    ) ELSE (
        echo "Could not find vcvarsall.bat. Is visual studio 2017 community edition installed?"
        goto:eof
    )
)

if %VSCMD_ARG_TGT_ARCH%==x86 (
    call "%VCINSTALLDIR%\Auxiliary\Build\vcvars64.bat"
)

set OE_CMAKE_EXE=%DevEnvDir%COMMONEXTENSIONS\MICROSOFT\CMAKE\CMake\bin\cmake.exe
set OE_NINJA_EXE=%DevEnvDir%COMMONEXTENSIONS\MICROSOFT\CMAKE\Ninja\ninja.exe

rem ******************************
rem MikktSpace and tinygltf
echo Creating and Building MikktSpace and tinygltf

setlocal EnableDelayedExpansion
set OE_GENERATE_BUILD_CONFIG=Debug
set OE_BUILD_DIR=%OE_ROOT%\thirdparty\cmake-ninjabuild-x64-!OE_GENERATE_BUILD_CONFIG!
IF NOT EXIST "!OE_BUILD_DIR!" md !OE_BUILD_DIR!
cd !OE_BUILD_DIR!
"%OE_CMAKE_EXE%" -G "Ninja" -DCMAKE_CXX_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_C_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_MAKE_PROGRAM="%OE_NINJA_EXE%" -DCMAKE_DEBUG_POSTFIX=_d -DCMAKE_INSTALL_PREFIX:PATH="%OE_ROOT%\thirdparty\amd64" -DCMAKE_BUILD_TYPE="!OE_GENERATE_BUILD_CONFIG!" "%OE_ROOT%/thirdparty"
ninja install

set OE_GENERATE_BUILD_CONFIG=Release
set OE_BUILD_DIR=%OE_ROOT%\thirdparty\cmake-ninjabuild-x64-!OE_GENERATE_BUILD_CONFIG!
IF NOT EXIST "!OE_BUILD_DIR!" md !OE_BUILD_DIR!
cd !OE_BUILD_DIR!
"%OE_CMAKE_EXE%" -G "Ninja" -DCMAKE_CXX_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_C_COMPILER:FILEPATH="%VCToolsInstallDir%bin\HostX64\x64\cl.exe" -DCMAKE_MAKE_PROGRAM="%OE_NINJA_EXE%" -DCMAKE_INSTALL_PREFIX:PATH="%OE_ROOT%\thirdparty\amd64" -DCMAKE_BUILD_TYPE="!OE_GENERATE_BUILD_CONFIG!" "%OE_ROOT%/thirdparty"
ninja install

endlocal

rem ******************************
rem DirectXTK
echo Building DirectXTK solution
cd %OE_ROOT%\thirdparty\DirectXTK
msbuild DirectXTK_Desktop_2017_Win10.sln /p:Configuration=Debug /p:Platform="x64"
msbuild DirectXTK_Desktop_2017_Win10.sln /p:Configuration=Release /p:Platform="x64"

rem ******************************
rem g3log
echo Creating and building g3log solution
set G3LOG_BUILD_PATH=%OE_ROOT%\thirdparty\g3log
IF EXIST %G3LOG_BUILD_PATH% (
    set NOTHING=0
) ELSE (
    md %G3LOG_BUILD_PATH%
)
cd %G3LOG_BUILD_PATH% 

rem Create the VS solutions and cmake init cache with common values
"%OE_CMAKE_EXE%" . -G "Visual Studio 15 2017" -A x64 -DCMAKE_DEBUG_POSTFIX=d -DCMAKE_INSTALL_PREFIX="%OE_ROOT%\thirdparty\amd64" -DG3_SHARED_LIB=OFF -DADD_FATAL_EXAMPLE=OFF

rem Build debug and release lib's
"%OE_CMAKE_EXE%" --build . --config Debug --target install -- /p:Configuration=Debug /p:Platform="x64"
rem "%OE_CMAKE_EXE%" --build . --config Release --target install -- /p:Configuration=Release /p:Platform="x64"

rem For some reason, the g3logger cmake find_module needs these directories to exist. Even though they are empty.
md "%OE_ROOT%\thirdparty\amd64\COMPONENT"
md "%OE_ROOT%\thirdparty\amd64\libraries"

rem ******************************
rem googletest
echo Creating and building googletest solution
set GOOGLETEST_BUILD_PATH=%OE_ROOT%\thirdparty\googletest\googletest
IF EXIST %GOOGLETEST_BUILD_PATH% (
    set NOTHING=0    
) ELSE (
    md %GOOGLETEST_BUILD_PATH%
)
cd %GOOGLETEST_BUILD_PATH% 

"%VSINSTALLDIR%COMMON7\IDE\COMMONEXTENSIONS\MICROSOFT\CMAKE\CMake\bin\cmake.exe" -DBUILD_SHARED_LIBS=ON -G "Visual Studio 15 2017" -A x64
msbuild gtest.sln /p:Configuration=Debug /p:Platform="x64"
msbuild gtest.sln /p:Configuration=Release /p:Platform="x64"

rem ******************************
rem googlemock
echo Creating and building googlemock solution
set GOOGLEMOCK_BUILD_PATH=%OE_ROOT%\thirdparty\googletest\googlemock
IF EXIST %GOOGLEMOCK_BUILD_PATH% (
    set NOTHING=0    
) ELSE (
    md %GOOGLEMOCK_BUILD_PATH%
)
cd %GOOGLEMOCK_BUILD_PATH% 

echo %cd%

"%VSINSTALLDIR%COMMON7\IDE\COMMONEXTENSIONS\MICROSOFT\CMAKE\CMake\bin\cmake.exe" -DBUILD_SHARED_LIBS=ON -G "Visual Studio 15 2017" -A x64
msbuild gmock.sln /p:Configuration=Debug /p:Platform="x64"
msbuild gmock.sln /p:Configuration=Release /p:Platform="x64"

endlocal
