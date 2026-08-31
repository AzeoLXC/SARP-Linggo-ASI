cmake_minimum_required(VERSION 3.20)
project(SARPLinggo VERSION 1.4 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".asi")

# ImGui
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx9.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)

# In-tree MinHook Sources
set(MINHOOK_SOURCES
    src/vendor/minhook/src/buffer.c
    src/vendor/minhook/src/hook.c
    src/vendor/minhook/src/trampoline.c
    src/vendor/minhook/src/hde/hde32.c
    src/vendor/minhook/src/hde/hde64.c
)

add_library(SARPLinggo SHARED
    src/main.cpp
    src/config.cpp
    src/translator.cpp
    src/chat_listener.cpp
    src/clipboard_listener.cpp
    src/overlay_gui.cpp
    src/d3d9_hook.cpp
    ${MINHOOK_SOURCES}
    ${IMGUI_SOURCES}
)

set_target_properties(SARPLinggo PROPERTIES OUTPUT_NAME "SARPLinggo")

target_include_directories(SARPLinggo PRIVATE
    src
    src/vendor
    src/vendor/minhook/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(SARPLinggo PRIVATE
    d3d9
    dwmapi
    winhttp
    bcrypt
    comdlg32
    gdi32
    user32
    shell32
    advapi32
)

target_compile_definitions(SARPLinggo PRIVATE _CRT_SECURE_NO_WARNINGS)

if(MSVC)
    target_compile_options(SARPLinggo PRIVATE /W3 /O2 /EHsc /permissive-)
else()
    target_compile_options(SARPLinggo PRIVATE -O3 -Wall -Wextra)
endif()
