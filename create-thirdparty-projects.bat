echo off
setlocal enableextensions

set G3LOG_BUILD_PATH=thirdparty\g3log
IF EXIST %G3LOG_BUILD_PATH% (
    set NOTHING=0    
) ELSE (
    md %G3LOG_BUILD_PATH%
)
cd %G3LOG_BUILD_PATH% 

if defined VSINSTALLDIR (
    "%VSINSTALLDIR%COMMON7\IDE\COMMONEXTENSIONS\MICROSOFT\CMAKE\CMake\bin\cmake.exe" -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 15 2017" -A x64
    msbuild g3log.sln /p:Configuration=Debug /t:g3logger /p:Platform="x64"
    msbuild g3log.sln /p:Configuration=Release /t:g3logger /p:Platform="x64"
) else (
    echo You should run this command from the "Developer Command Prompt for VS 2017"
)
endlocal
