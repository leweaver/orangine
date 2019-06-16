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
cmake . -G "Visual Studio 15 2017" -A x64 -DCMAKE_DEBUG_POSTFIX=d -DCMAKE_INSTALL_PREFIX="%OE_ROOT%\thirdparty\amd64" -DG3_SHARED_LIB=OFF -DADD_FATAL_EXAMPLE=OFF

rem Build debug and release lib's
cmake --build . --config Debug --target install -- /p:Configuration=Debug /p:Platform="x64"
cmake --build . --config Release --target install -- /p:Configuration=Release /p:Platform="x64"

rem ******************************
rem tinygltf
echo Copying tinygltf files
set TINYGLTF_BUILD_PATH=%OE_ROOT%\thirdparty\tinygltf
IF EXIST %TINYGLTF_BUILD_PATH% (
    set NOTHING=0
) ELSE (
    md %TINYGLTF_BUILD_PATH%
)
cd %TINYGLTF_BUILD_PATH% 

IF NOT EXIST %OE_ROOT%\thirdparty\amd64\include\tinygltf (
    md %OE_ROOT%\thirdparty\amd64\include\tinygltf
)
xcopy /D /Y %TINYGLTF_BUILD_PATH%\*.h %OE_ROOT%\thirdparty\amd64\include\tinygltf
xcopy /D /Y %TINYGLTF_BUILD_PATH%\json.hpp %OE_ROOT%\thirdparty\amd64\include\tinygltf


rem 
rem echo Creating and building googletest solution
rem set GOOGLETEST_BUILD_PATH=%OE_ROOT%\thirdparty\googletest\googletest
rem IF EXIST %GOOGLETEST_BUILD_PATH% (
rem     set NOTHING=0    
rem ) ELSE (
rem     md %GOOGLETEST_BUILD_PATH%
rem )
rem cd %GOOGLETEST_BUILD_PATH% 
rem 
rem "%VSINSTALLDIR%COMMON7\IDE\COMMONEXTENSIONS\MICROSOFT\CMAKE\CMake\bin\cmake.exe" -DBUILD_SHARED_LIBS=ON -G "Visual Studio 15 2017" -A x64
rem msbuild gtest.sln /p:Configuration=Debug /p:Platform="x64"
rem msbuild gtest.sln /p:Configuration=Release /p:Platform="x64"
rem 
rem echo Creating and building googlemock solution
rem set GOOGLEMOCK_BUILD_PATH=%OE_ROOT%\thirdparty\googletest\googlemock
rem IF EXIST %GOOGLEMOCK_BUILD_PATH% (
rem     set NOTHING=0    
rem ) ELSE (
rem     md %GOOGLEMOCK_BUILD_PATH%
rem )
rem cd %GOOGLEMOCK_BUILD_PATH% 
rem 
rem echo %cd%
rem 
rem "%VSINSTALLDIR%COMMON7\IDE\COMMONEXTENSIONS\MICROSOFT\CMAKE\CMake\bin\cmake.exe" -DBUILD_SHARED_LIBS=ON -G "Visual Studio 15 2017" -A x64
rem msbuild gmock.sln /p:Configuration=Debug /p:Platform="x64"
rem msbuild gmock.sln /p:Configuration=Release /p:Platform="x64"
rem 
endlocal
