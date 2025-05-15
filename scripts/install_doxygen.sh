#!/bin/bash
set -e

if command -v doxygen >/dev/null 2>&1; then
    echo "Doxygen is already installed."
else
    echo "Installing Doxygen..."
    sudo apt-get update
    sudo apt-get install -y doxygen
fi