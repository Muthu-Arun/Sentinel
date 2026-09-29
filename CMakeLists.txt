cmake_minimum_required(VERSION 3.20)
project(Sentinel LANGUAGES CXX)

# 1. System core dependencies
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
endif()

include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
if(UNIX AND NOT APPLE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(glfw)

# Fetch CURL
FetchContent_Declare(
    curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG curl-8_7_1
)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(curl)

# Fetch Drogon
FetchContent_Declare(
    drogon
    GIT_REPOSITORY https://github.com/drogonframework/drogon.git
    GIT_TAG v1.9.8
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_CTL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(drogon)
# 2. Define ImGui Sources
# We need the core files AND the specific backends for GLFW and OpenGL3
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)

option(USE_TSAN "Enable ThreadSanitizer" OFF)
option(USE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# 2. Prevent enabling both at the same time
if (USE_TSAN AND USE_ASAN)
    message(FATAL_ERROR "You cannot enable both TSan and ASan at the same time. Choose one.")
endif()

# 3. Setup compiler and linker flags based on the selection
if ((USE_TSAN OR USE_ASAN) AND MSVC)
    message(FATAL_ERROR "Sanitizer options are only supported with GCC or Clang")
elseif (USE_TSAN)
    message(STATUS "ThreadSanitizer (TSan) Enabled")
    
    # -fsanitize=thread: Enables TSan
    # -g: Generates debug symbols (crucial for line numbers in errors)
    # -O1: Minimum optimization level recommended for TSan to work well
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -g -O1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")

elseif (USE_ASAN)
    message(STATUS "AddressSanitizer (ASan) and UBSan Enabled")
    
    # -fsanitize=address,undefined: Enables ASan and UBSan
    # -fno-omit-frame-pointer: Ensures perfect stack traces in error logs
    # -g: Generates debug symbols
    # -O1: Keeps performance reasonable while finding bugs
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -g -O1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
endif()

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
set(SIMDJSON_SOURCES
    ${VENDOR_DIR}/simdjson/simdjson.cpp
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
set(SSE_SOURCES
    src/sse/sse.cpp)

# 3. Create the Executable
add_executable(sentinel src/main.cpp ${IMGUI_SOURCES} ${HTTP_SOURCES} ${WIDGET_SOURCES} ${WINDOW_SOURCES} ${PARSING_SOURCES} ${IM_PLOT_SOURCES} ${UTIL_SOURCES}
                                        ${SSE_SOURCES} ${SIMDJSON_SOURCES})

set_target_properties(sentinel PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_options(sentinel PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra>
)

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
    ${SRC_DIR}/sse
    ${VENDOR_DIR}/implot
    ${VENDOR_DIR}/stb
    ${VENDOR_DIR}/simdjson
)

# 5. Link Libraries
target_link_libraries(sentinel PRIVATE
    glfw
    OpenGL::GL
    drogon
    Threads::Threads
    libcurl
)
if(UNIX AND NOT APPLE)
    target_link_libraries(sentinel PRIVATE X11::X11)
endif()
if(WIN32)
    target_link_libraries(sentinel PRIVATE ws2_32 rpcrt4 bcrypt crypt32 iphlpapi uuid)
endif()

# Keep runtime files next to the executable for single and multi-config generators.
add_custom_command(TARGET sentinel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/fonts" "$<TARGET_FILE_DIR:sentinel>/fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/sentinel.json" "$<TARGET_FILE_DIR:sentinel>/sentinel.json"
)
