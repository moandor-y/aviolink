@echo off
setlocal enabledelayedexpansion

:: Use vswhere.exe to locate VS (ships with all VS2017+ installs)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VSWHERE!" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

set "FOUND_VCVARS="
if exist "!VSWHERE!" (
    for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "FOUND_VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
    )
)

:: Fallback: scan common VS 2019/2022 editions
if not exist "!FOUND_VCVARS!" (
    for %%v in (2022 2019) do for %%e in (Community Professional Enterprise BuildTools) do (
        if not exist "!FOUND_VCVARS!" (
            set "_p=%ProgramFiles%\Microsoft Visual Studio\%%v\%%e\VC\Auxiliary\Build\vcvars64.bat"
            if exist "!_p!" set "FOUND_VCVARS=!_p!"
        )
    )
)

endlocal & set "VCVARS_PATH=%FOUND_VCVARS%"

if not exist "%VCVARS_PATH%" (
    echo Error: vcvars64.bat not found at "%VCVARS_PATH%"
    exit /b 1
)

echo Initializing MSVC Compiler Environment...
call "%VCVARS_PATH%" >nul

echo Compiling Tests...
cl /EHsc /Imock_arduino /I. /Fo:tests\ tests\test_runner.cpp /Tpmain.ino /Fe:tests\run_tests.exe

if %ERRORLEVEL% neq 0 (
    echo Compilation Failed!
    exit /b 1
)

echo Running Tests...
tests\run_tests.exe
