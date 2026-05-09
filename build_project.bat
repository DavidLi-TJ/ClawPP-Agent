@echo off
cd /d C:\david\Qt\cpqclaw
if not exist build mkdir build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug -j8
