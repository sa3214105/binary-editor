@echo off
where doxygen >nul 2>nul
if %ERRORLEVEL%==0 (
    echo Doxygen is already installed.
    exit /b 0
)
echo Downloading Doxygen...
powershell -Command "Invoke-WebRequest -Uri https://www.doxygen.nl/files/doxygen-1.10.0.windows.x64.bin.zip -OutFile doxygen.zip"
powershell -Command "Expand-Archive -Path doxygen.zip -DestinationPath .\doxygen"
set PATH=%CD%\doxygen\doxygen-1.10.0\bin;%PATH%
echo Doxygen installed to doxygen\doxygen-1.10.0\bin. Please add it to your PATH if needed.