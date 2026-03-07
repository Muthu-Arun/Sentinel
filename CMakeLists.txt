cmake_minimum_required(VERSION 3.20)
project(Sentinel)

set(CMAKE_CXX_STANDARD 23)
# add_compile_options(-Wall -Wextra -Wpedantic -Werror)
add_compile_options(-Wall -Wextra )
# 1. Find system dependencies (GLFW and OpenGL)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Drogon REQUIRED)
# 2. Define ImGui Sources
# We need the core files AND the specific backends for GLFW and OpenGL3
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
set(IM_PLOT_SOURCES
    ${VENDOR_DIR}/implot/implot_items.cpp
    ${VENDOR_DIR}/implot/implot.cpp
)
set(HTTP_SOURCES
    src/http/poll.cpp)
set(WIDGET_SOURCES
    src/widget/widget.cpp)
set(WINDOW_SOURCES
    src/window/window.cpp)
set(PARSING_SOURCES
    src/parsenrender/json.cpp
    src/render/init.cpp)
set(UTIL_SOURCES
    src/utils/util.cpp)
# 3. Create the Executable
add_executable(sentinel src/main.cpp ${IMGUI_SOURCES} ${HTTP_SOURCES} ${WIDGET_SOURCES} ${WINDOW_SOURCES} ${PARSING_SOURCES} ${IM_PLOT_SOURCES} ${UTIL_SOURCES})

# 4. Include Directories
# Allow includes like <imgui.h> and <backends/imgui_impl_glfw.h>
target_include_directories(sentinel PUBLIC 
    ${IMGUI_DIR} 
    ${IMGUI_DIR}/backends
    ${SRC_DIR}/http
    ${SRC_DIR}/widget
    ${SRC_DIR}/window
    ${SRC_DIR}/render
    ${SRC_DIR}/utils
    ${SRC_DIR}/parsenrender
    ${VENDOR_DIR}/implot
    ${VENDOR_DIR}/stb
)

# 5. Link Libraries
target_link_libraries(sentinel PRIVATE glfw OpenGL::GL Drogon::Drogon pthread)