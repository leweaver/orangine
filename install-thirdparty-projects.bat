@echo off
setlocal enableextensions
setlocal EnableDelayedExpansion

rem ******************************
rem Load Environment
rem ******************************

set "OE_ROOT=%cd%"
set "OE_ORIG_PATH=%PATH%"
set "PYTHON_32_EXE=%LOCALAPPDATA%\Programs\Python\Python37-32\python.exe"
set "PYTHON_64_EXE=%LOCALAPPDATA%\Programs\Python\Python37\python.exe"

if not exist %PYTHON_32_EXE% (
    echo "Could not find !PYTHON_32_EXE!. Is it installed?"
    goto:eof
)
if not exist %PYTHON_64_EXE% (
    echo "Could not find !PYTHON_64_EXE!. Is it installed?"
    goto:eof
)

if not defined VSINSTALLDIR (
    powershell -command "Get-VSSetupInstance" > NUL
    rem If this failed, run the following in a regular powershell window:
    rem Install-Module VSSetup -Scope CurrentUser
    call :oe_verify_errorlevel "Detect Get-VSSetupInstance powershell utility" || goto:eof

    for /f "tokens=*" %%a in ('powershell -command "Get-VSSetupInstance | Select-VSSetupInstance -Require Microsoft.VisualStudio.Workload.NativeDesktop -Latest | Select-Object -ExpandProperty InstallationPath"') do @set "OE_VC_INSTALL_PATH=%%a"

    IF EXIST "!OE_VC_INSTALL_PATH!\VC\Auxiliary\Build\vcvars64.bat" (
        echo Using visual studio at: !OE_VC_INSTALL_PATH!
        set VCINSTALLDIR=!OE_VC_INSTALL_PATH!\VC\
    ) ELSE (
        echo "Could not find vcvars64.bat. Is VSSetup powershell module, and visual studio community edition installed?"
        goto:eof
    )
)

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
rem Python environments
rem ******************************
if not exist "%OE_ROOT%\thirdparty\pyenv_37_x64" (
    rem If this fails with: 
    rem   No such file or directory: 'C:\\Users\\hotma\\AppData\\Local\\Programs\\Python\\Python37\\lib\\venv\\scripts\\nt\\python_d.exe'
    rem upgrade your version of python to at least 3.7.4. (3.7.3 has a bug)
    %PYTHON_64_EXE% -m venv %OE_ROOT%\thirdparty\pyenv_37_x64
    call :oe_verify_errorlevel "Create pyenv_37_x64" || goto:eof
    
    call :init_pyenv pyenv_37_x64 || goto:eof
)
if not exist "%OE_ROOT%\thirdparty\pyenv_37_x86" (
    rem If this fails with: 
    rem   No such file or directory: 'C:\\Users\\hotma\\AppData\\Local\\Programs\\Python\\Python37\\lib\\venv\\scripts\\nt\\python_d.exe'
    rem upgrade your version of python to at least 3.7.4. (3.7.3 has a bug)
    %PYTHON_32_EXE% -m venv %OE_ROOT%\thirdparty\pyenv_37_x86
    call :oe_verify_errorlevel "Create pyenv_37_x86" || goto:eof
    
    call :init_pyenv pyenv_37_x86 || goto:eof
)

rem enable this if you are a madman and want to run the engine against a debug build of python
if "" == "false" (
    set "PYTHON_DEBUG_64_EXE=C:\Users\hotma\AppData\Local\Programs\Python\Python37\python_d.exe"
    if not exist "%OE_ROOT%\thirdparty\pyenv_37_d" (
        %PYTHON_DEBUG_64_EXE% -m venv %OE_ROOT%\thirdparty\pyenv_37_d
    )
    %OE_ROOT%\thirdparty\pyenv_37_d\Scripts\activate.bat
    rem Need to install numpy with build isolation, so that we can use the debug version of cython.
    %PYTHON_DEBUG_64_EXE% -m pip install --no-binary :all: --global-option build --global-option --debug wheel
    %PYTHON_DEBUG_64_EXE% -m pip install --no-binary :all: --global-option build --global-option --debug cython==0.29.14
    %PYTHON_DEBUG_64_EXE% -m pip install --no-binary :all: --global-option build --global-option --debug --no-build-isolation numpy==1.18.0

    %PYTHON_DEBUG_64_EXE% -m pip install --no-binary :all: --global-option build --global-option --debug ptvsd==4.3.2
    %PYTHON_DEBUG_64_EXE% -m pip install --no-binary :all: --global-option build --global-option --debug vectormath==0.2.2
    %PYTHON_DEBUG_64_EXE% -m pip install --no-binary :all: --global-option build --global-option --debug jsonschema==3.0.2
    %PYTHON_DEBUG_64_EXE% -m pip install --no-binary :all: --global-option build --global-option --debug argparse==1.4.0
    %OE_ROOT%\thirdparty\pyenv_37_d\Scripts\deactivate.bat
)

rem ******************************
rem Top of the build tree
rem ******************************

setlocal
call :build_all x86 || goto:eof
endlocal

setlocal
call :build_all x64 || goto:eof
endlocal

rem ******************************
rem End of main script.
rem ******************************

goto:eof



rem *****************************
rem !!!       Build All       !!!
rem *****************************

:init_pyenv
rem takes 1 arg; py env name
setlocal
set "OE_PYENV=%~1"
set "OE_PYENV_LOGFILE=%OE_ROOT%\thirdparty\%OE_PYENV%\init_pyenv.log"

call %OE_ROOT%\thirdparty\%OE_PYENV%\Scripts\activate.bat
call :oe_verify_errorlevel "%OE_PYENV%\Scripts\activate.bat" || EXIT /B 1

echo "Installing modules via PIP " > %OE_PYENV_LOGFILE%
pip install numpy==1.18.0 >> %OE_PYENV_LOGFILE% 2>&1
call :oe_verify_errorlevel "%OE_PYENV% pip install numpy" || EXIT /B 1
pip install ptvsd==4.3.2 >> %OE_PYENV_LOGFILE% 2>&1
call :oe_verify_errorlevel "%OE_PYENV% pip install ptvsd" || EXIT /B 1
pip install vectormath==0.2.2 >> %OE_PYENV_LOGFILE% 2>&1
call :oe_verify_errorlevel "%OE_PYENV% pip install vectormath" || EXIT /B 1
pip install jsonschema==3.0.2 >> %OE_PYENV_LOGFILE% 2>&1
call :oe_verify_errorlevel "%OE_PYENV% pip install jsonschema" || EXIT /B 1
pip install argparse==1.4.0 >> %OE_PYENV_LOGFILE% 2>&1
call :oe_verify_errorlevel "%OE_PYENV% pip install argparse" || EXIT /B 1

call %OE_ROOT%\thirdparty\%OE_PYENV%\Scripts\deactivate.bat

endlocal
EXIT /B 0

:build_all
rem Takes 1 arg; architecture
set "CM_ARCHITECTURE=%~1"

call :init_python || EXIT /B 1

if not "%CM_ARCHITECTURE%"=="%VSCMD_ARG_TGT_ARCH%" (
    if "%CM_ARCHITECTURE%"=="x64" (
        call "%VCINSTALLDIR%Auxiliary\Build\vcvars64.bat"
    ) else (
        call "%VCINSTALLDIR%Auxiliary\Build\vcvars32.bat"
    )
)

set "OE_CMAKE_EXE=%DevEnvDir%COMMONEXTENSIONS\MICROSOFT\CMAKE\CMake\bin\cmake.exe"
set "OE_NINJA_EXE=%DevEnvDir%COMMONEXTENSIONS\MICROSOFT\CMAKE\Ninja\ninja.exe"
set "OE_INSTALL_PREFIX=%OE_ROOT%\bin\%CM_ARCHITECTURE%"

rem pybind11
call :cmake_ninja_install "%OE_ROOT%\thirdparty\pybind11", pybind11 || EXIT /B 1

rem g3log
call :cmake_ninja_install "%OE_ROOT%\thirdparty\g3log", g3log || EXIT /B 1

rem MikktSpace and tinygltf are build using our own CMakeLists file.
call :cmake_ninja_install "%OE_ROOT%\thirdparty", thirdparty || EXIT /B 1

rem Google Test
call :cmake_msvc_shared "%OE_ROOT%\thirdparty\googletest\googletest", gtest || EXIT /B 1

rem Google Mock
call :cmake_msvc_shared "%OE_ROOT%\thirdparty\googletest\googlemock", gmock || EXIT /B 1

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
    EXIT /B 1
)
if %CM_ARCHITECTURE% == "x86" (
    set CM_GENERATOR_ARCHITECTURE=Win32
) else (
    set CM_GENERATOR_ARCHITECTURE=x64
)
msbuild %OE_DXTK_SLN%.sln /p:Configuration=Debug /p:Platform="%CM_GENERATOR_ARCHITECTURE%" > %OE_DIRECTXTK_LOGFILE%
call :oe_verify_errorlevel "msbuild DirectXTK Debug" || EXIT /B 1
msbuild %OE_DXTK_SLN%.sln /p:Configuration=Release /p:Platform="%CM_GENERATOR_ARCHITECTURE%" >> %OE_DIRECTXTK_LOGFILE%
call :oe_verify_errorlevel "msbuild DirectXTK Release" || EXIT /B 1


rem Hacky Steps:
rem For some reason, the g3logger cmake find_module needs these directories to exist. Even though they are empty.
IF NOT EXIST "%OE_INSTALL_PREFIX%\Debug\COMPONENT" md "%OE_INSTALL_PREFIX%\Debug\COMPONENT"
IF NOT EXIST "%OE_INSTALL_PREFIX%\Release\COMPONENT" md "%OE_INSTALL_PREFIX%\Release\COMPONENT"
IF NOT EXIST "%OE_INSTALL_PREFIX%\Debug\libraries" md "%OE_INSTALL_PREFIX%\Debug\libraries"
IF NOT EXIST "%OE_INSTALL_PREFIX%\Release\libraries" md "%OE_INSTALL_PREFIX%\Release\libraries"

EXIT /B 0



:init_python
if "%CM_ARCHITECTURE%" == "x64" ( 
    set PY_ARCHITECTURE=64
    set CM_EXPECTED_PYTHON_PATH=%LOCALAPPDATA%\Programs\Python\Python37\
) else (
    set PY_ARCHITECTURE=32
    set CM_EXPECTED_PYTHON_PATH=%LOCALAPPDATA%\Programs\Python\Python37-32\
)
set "PATH=%CM_EXPECTED_PYTHON_PATH%;%CM_EXPECTED_PYTHON_PATH%Scripts\;%PATH%"

rem Check that the correct python version is installed for this architecture.
for /f "tokens=*" %%a in ('python -c "exec(""import platform\n(bits,linkage)=platform.architecture()\n(major,minor,patch)=platform.python_version_tuple()\nprint('Python',major,bits,'OK' if int(major)==3 and int(minor)>=6 else 'TOO_OLD')"")"') do @set "PYTHON_VERSION_OK=%%a"
for /f "tokens=*" %%a in ('python --version') do @set "PYTHON_VERSION=%%a"
if NOT "%PYTHON_VERSION_OK%" == "Python 3 %PY_ARCHITECTURE%bit OK" (
    echo * detected major version/arch: %PYTHON_VERSION_OK%
    echo * detected minor version: %PYTHON_VERSION%
    echo * [[31mFailed[0m] Invalid python version. Must have 3.6 %PY_ARCHITECTURE% bit or greater at %CM_EXPECTED_PYTHON_PATH%.
    EXIT /B 1
) else (
    echo * [[32mOK[0m] Python version: %PYTHON_VERSION%, %PYTHON_VERSION_OK%
)
rem check that pip is installed
python -m pip -V > NUL
call :oe_verify_errorlevel "check python PIP is installed" || EXIT /B 1

rem install pytest if it's missing
python -m pip show pytest > NUL
if "%errorlevel%" == "1" (
    python -m pip install pytest > NUL
    call :oe_verify_errorlevel "python -m pip install pytest" || EXIT /B 1
) else (
    rem this should always succeed, just using it for a nicely formatted OK message
    call :oe_verify_errorlevel "check pytest installed"
)

EXIT /B 0





:cmake_ninja_install
rem Takes 2 args; source path, target name

set "CM_NINJA_SOURCE_PATH=%~1"
set "CM_TARGET_NAME=%~2"

set OE_GENERATE_BUILD_CONFIG=Debug
set "CM_NINJA_BUILD_PATH=%OE_ROOT%\thirdparty\cmake-%CM_TARGET_NAME%-ninjabuild-%CM_ARCHITECTURE%-!OE_GENERATE_BUILD_CONFIG!"
IF NOT EXIST "!CM_NINJA_BUILD_PATH!" md !CM_NINJA_BUILD_PATH!
cd "!CM_NINJA_BUILD_PATH!"
set "CM_LOGFILE=%CM_NINJA_BUILD_PATH%\oe-build-log.txt"
echo %CM_TARGET_NAME% %OE_GENERATE_BUILD_CONFIG% build output: %CM_LOGFILE%
IF EXIST "!CM_NINJA_BUILD_PATH!\CMakeCache.txt" del "!CM_NINJA_BUILD_PATH!\CMakeCache.txt"
"%OE_CMAKE_EXE%" -G "Ninja" -DCMAKE_CXX_COMPILER:FILEPATH="%VCToolsInstallDir%bin\Host%CM_ARCHITECTURE%\%CM_ARCHITECTURE%\cl.exe" -DCMAKE_MAKE_PROGRAM="%OE_NINJA_EXE%" -DCMAKE_DEBUG_POSTFIX=_d -DCMAKE_INSTALL_PREFIX:PATH="%OE_INSTALL_PREFIX%/%OE_GENERATE_BUILD_CONFIG%" -DCMAKE_BUILD_TYPE="!OE_GENERATE_BUILD_CONFIG!" "%CM_NINJA_SOURCE_PATH%" > %CM_LOGFILE%
call :oe_verify_errorlevel "cmake" || EXIT /B 1

ninja install >> %CM_LOGFILE%
call :oe_verify_errorlevel "ninja install" || EXIT /B 1

set OE_GENERATE_BUILD_CONFIG=Release
set "CM_NINJA_BUILD_PATH=%OE_ROOT%\thirdparty\cmake-%CM_TARGET_NAME%-ninjabuild-%CM_ARCHITECTURE%-!OE_GENERATE_BUILD_CONFIG!"
IF NOT EXIST "!CM_NINJA_BUILD_PATH!" md !CM_NINJA_BUILD_PATH!
cd !CM_NINJA_BUILD_PATH!
set "CM_LOGFILE=%CM_NINJA_BUILD_PATH%\oe-build-log.txt"
echo %CM_TARGET_NAME% %OE_GENERATE_BUILD_CONFIG% build output: %CM_LOGFILE%
IF EXIST "!CM_NINJA_BUILD_PATH!\CMakeCache.txt" del "!CM_NINJA_BUILD_PATH!\CMakeCache.txt"
"%OE_CMAKE_EXE%" -G "Ninja" -DCMAKE_CXX_COMPILER:FILEPATH="%VCToolsInstallDir%bin\Host%CM_ARCHITECTURE%\%CM_ARCHITECTURE%\cl.exe" -DCMAKE_MAKE_PROGRAM="%OE_NINJA_EXE%" -DCMAKE_INSTALL_PREFIX:PATH="%OE_INSTALL_PREFIX%/%OE_GENERATE_BUILD_CONFIG%" -DCMAKE_BUILD_TYPE="!OE_GENERATE_BUILD_CONFIG!" "%CM_NINJA_SOURCE_PATH%" > %CM_LOGFILE%
call :oe_verify_errorlevel "cmake" || EXIT /B 1

ninja install >> %CM_LOGFILE%
call :oe_verify_errorlevel "ninja install" || EXIT /B 1

EXIT /B 0





:cmake_msvc_shared
rem Takes 2 args; source path, target name
set "CM_MSVC_SOURCE_PATH=%~1"
set "CM_TARGET_NAME=%~2"

set "CM_MSVC_BUILD_PATH=%OE_ROOT%\thirdparty\cmake-%CM_TARGET_NAME%-build-%CM_ARCHITECTURE%"

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

if %CM_ARCHITECTURE% == "x86" (
    set CM_GENERATOR_ARCHITECTURE=Win32
) else (
    set CM_GENERATOR_ARCHITECTURE=x64
)

"%OE_CMAKE_EXE%" -DBUILD_SHARED_LIBS=ON -G "%OE_CMAKE_GENERATE%" -A %CM_GENERATOR_ARCHITECTURE% "%CM_MSVC_SOURCE_PATH%" > %CM_LOGFILE%
call :oe_verify_errorlevel "cmake" || EXIT /B 1

msbuild %CM_TARGET_NAME%.sln /p:Configuration=Debug /p:Platform="%CM_GENERATOR_ARCHITECTURE%" >> %CM_LOGFILE%
call :oe_verify_errorlevel "msbuild Debug" || EXIT /B 1

msbuild %CM_TARGET_NAME%.sln /p:Configuration=Release /p:Platform="%CM_GENERATOR_ARCHITECTURE%" >> %CM_LOGFILE%
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