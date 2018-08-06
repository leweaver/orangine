rem echo off
setlocal enableextensions

set "OE_ROOT=%cd%"

if not defined VSINSTALLDIR (
    echo You should run this command from the "Developer Command Prompt for VS 2017"
    goto:eof
)
"%VSINSTALLDIR%COMMON7\IDE\COMMONEXTENSIONS\MICROSOFT\CMAKE\CMake\bin\cmake.exe" -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 15 2017" -A x64
msbuild g3log.sln /p:Configuration=Debug /t:g3logger /p:Platform="x64"
msbuild g3log.sln /p:Configuration=Release /t:g3logger /p:Platform="x64"

echo Creating and building g3log solution
set G3LOG_BUILD_PATH=%OE_ROOT%\thirdparty\g3log
IF EXIST %G3LOG_BUILD_PATH% (
    set NOTHING=0    
) ELSE (
    md %G3LOG_BUILD_PATH%
)
cd %G3LOG_BUILD_PATH% 

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
