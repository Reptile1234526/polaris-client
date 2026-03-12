cmake_minimum_required(VERSION 3.20)
project(PolarisClient CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Vendor paths ──────────────────────────────────────────────────────────────
set(IMGUI_DIR  ${CMAKE_SOURCE_DIR}/vendor/imgui)
set(MINHOOK_DIR ${CMAKE_SOURCE_DIR}/vendor/minhook)

# ── ImGui sources ─────────────────────────────────────────────────────────────
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx12.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
)

# ── Client sources ────────────────────────────────────────────────────────────
file(GLOB_RECURSE CLIENT_SOURCES "src/*.cpp")

add_library(PolarisClient SHARED
    ${CLIENT_SOURCES}
    ${IMGUI_SOURCES}
)

target_include_directories(PolarisClient PRIVATE
    src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${MINHOOK_DIR}/include
)

target_link_libraries(PolarisClient PRIVATE
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
    dwmapi.lib
    ${MINHOOK_DIR}/lib/libMinHook.x64.lib
)

target_compile_definitions(PolarisClient PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

set_target_properties(PolarisClient PROPERTIES
    OUTPUT_NAME "PolarisClient"
    SUFFIX ".dll"
)

# ── Injector ──────────────────────────────────────────────────────────────────
add_executable(PolarisInjector injector/main.cpp)
target_compile_definitions(PolarisInjector PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
