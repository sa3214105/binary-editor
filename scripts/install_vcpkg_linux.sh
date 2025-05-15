#!/bin/bash
set -e

if [ ! -d "./vcpkg" ]; then
    echo "Cloning vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
    cd ..
else
    echo "vcpkg already exists."
fi