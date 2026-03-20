#!/usr/bin/env bash
# Clean reconfigure and build. Use this if you see:
#   CMake Error: Directory Information file not found
# (e.g. after switching CMake generators or a broken build tree.)
set -e
cd "$(dirname "$0")/.."
rm -rf build
cmake -G "Unix Makefiles" -B build
cmake --build build
