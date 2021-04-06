cmake_minimum_required(VERSION 3.17)
project(MonokumaHook)

set(CMAKE_CXX_STANDARD 20)

find_path(DETOURS_INCLUDE_DIR detours/detours.h)
find_library(DETOURS_LIBRARY detours)
include_directories(${DETOURS_INCLUDE_DIR})
link_libraries(${DETOURS_LIBRARY})

include_directories($ENV{DXSDK_DIR}/Include)
link_directories($ENV{DXSDK_DIR}/Lib/x86)

find_package(minhook CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_library(MonokumaHook SHARED
        Monokuma.cpp
        Monokuma.h
        kiero/kiero.cpp
        kiero/kiero.h
        kiero/examples/imgui/impl/win32_impl.cpp
        kiero/examples/imgui/impl/win32_impl.h
        kiero/examples/imgui/imgui/examples/imgui_impl_win32.cpp
        kiero/examples/imgui/imgui/examples/imgui_impl_win32.h
        kiero/examples/imgui/imgui/examples/imgui_impl_dx9.cpp
        kiero/examples/imgui/imgui/examples/imgui_impl_dx9.h)

target_link_libraries(MonokumaHook PRIVATE minhook::minhook)
target_link_libraries(MonokumaHook PRIVATE imgui::imgui)


