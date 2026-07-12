@echo off
setlocal
rem Build Game Dev Tycoon (standard C, CP949 sources)
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do set VSPATH=%%i
if "%VSPATH%"=="" (
    echo Visual Studio not found.
    exit /b 1
)
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /W4 /std:c17 /source-charset:.949 /execution-charset:.949 *.c /Fe:Tycoon.exe
