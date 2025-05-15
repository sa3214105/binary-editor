@echo off
if not exist "vcpkg" (
    echo Cloning vcpkg...
    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    call bootstrap-vcpkg.bat
    cd ..
) else (
    echo vcpkg already exists.
)