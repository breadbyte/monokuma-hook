cmake_minimum_required(VERSION 3.17)
project(MonokumaHook)

set(CMAKE_CXX_STANDARD 20)

find_path(DETOURS_INCLUDE_DIR detours/detours.h)
find_library(DETOURS_LIBRARY detours)
include_directories(${DETOURS_INCLUDE_DIR})
link_libraries(${DETOURS_LIBRARY})

find_package(imgui CONFIG REQUIRED)
include_directories($ENV{DXSDK_DIR}/Include)
link_directories($ENV{DXSDK_DIR}/Lib/x86)

add_library(MonokumaHook SHARED Monokuma.cpp Monokuma.h)

target_link_libraries(MonokumaHook PRIVATE imgui::imgui)