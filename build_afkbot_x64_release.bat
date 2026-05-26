@echo off
setlocal
cd /d "%~dp0"

where msbuild >nul 2>nul
if %errorlevel%==0 (
    msbuild AFKbotLoader.sln /m /p:Configuration=Release /p:Platform=x64
    exit /b %errorlevel%
)

if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" (
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" AFKbotLoader.sln /m /p:Configuration=Release /p:Platform=x64
    exit /b %errorlevel%
)

if exist "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" (
    "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" AFKbotLoader.sln /m /p:Configuration=Release /p:Platform=x64
    exit /b %errorlevel%
)

echo MSBuild not found. Open AFKbotLoader.sln in Visual Studio 2022 and build Release x64.
exit /b 1
