cmake_minimum_required(VERSION 3.20)
project(SARPLinggo VERSION 1.3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".asi")

# MinHook
include(FetchContent)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        v1.3.3
)
FetchContent_MakeAvailable(minhook)

# ImGui
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

add_library(SARPLinggo SHARED
    src/main.cpp
    src/config.cpp
    src/licensing.cpp
    src/translator.cpp
    src/chat_listener.cpp
    src/clipboard_listener.cpp
    src/overlay_gui.cpp
    src/d3d9_hook.cpp
    ${IMGUI_SOURCES}
)

set_target_properties(SARPLinggo PROPERTIES OUTPUT_NAME "SARPLinggo")

target_include_directories(SARPLinggo PRIVATE
    src
    src/vendor
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${minhook_SOURCE_DIR}/include
)

target_link_libraries(SARPLinggo PRIVATE
    minhook
    d3d9
    d3dx9
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
    target_compile_options(SARPLinggo PRIVATE /W3 /O2 /EHsc)
else()
    target_compile_options(SARPLinggo PRIVATE -O3 -Wall -Wextra)
endif()
