@echo off
echo [Polaris] Cloning vendor dependencies...

if not exist vendor\imgui (
    git clone https://github.com/ocornut/imgui vendor\imgui
)
if not exist vendor\minhook (
    git clone https://github.com/TsudaKageyu/minhook vendor\minhook
)

echo [Polaris] Configuring CMake (x64 Release)...
cmake -B build -A x64 -DCMAKE_BUILD_TYPE=Release

echo [Polaris] Building...
cmake --build build --config Release

echo [Polaris] Done! Output: build\Release\PolarisClient.dll
pause
