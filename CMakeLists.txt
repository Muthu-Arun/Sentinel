cmake_minimum_required(VERSION 3.20)
project(Sentinel LANGUAGES C CXX)

# Use static MSVC runtime (/MT, /MTd) on Windows so VC++ Redistributable DLLs are not required
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# 1. System core dependencies
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
endif()

include(FetchContent)

# Prefer static archives (.a / .lib) for OpenSSL, ZLIB, and UUID when building standalone portable binaries
option(SENTINEL_PREFER_SYSTEM_DEPS "Prefer vcpkg/system shared packages over FetchContent when available" OFF)
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    set(SENTINEL_PREFER_SYSTEM_DEPS ON CACHE BOOL "" FORCE)
endif()

if(NOT SENTINEL_PREFER_SYSTEM_DEPS)
    set(OPENSSL_USE_STATIC_LIBS TRUE CACHE BOOL "" FORCE)
    set(USE_STATIC_LIBS_ONLY ON CACHE BOOL "" FORCE)
endif()

if(SENTINEL_PREFER_SYSTEM_DEPS)
    find_package(glfw3 CONFIG QUIET)
    find_package(CURL CONFIG QUIET)
    if(NOT CURL_FOUND)
        find_package(CURL QUIET)
    endif()
    find_package(Drogon CONFIG QUIET)
endif()

# Fetch GLFW if not already provided by vcpkg/system
if(NOT glfw3_FOUND AND NOT TARGET glfw)
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
endif()

# Ensure ZLIB is available (statically if possible, or fetched via FetchContent)
if(NOT Drogon_FOUND)
    if(NOT WIN32)
        set(_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
        find_package(ZLIB QUIET)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
    else()
        find_package(ZLIB QUIET)
    endif()
    if(NOT ZLIB_FOUND)
        FetchContent_Declare(
            zlib
            GIT_REPOSITORY https://github.com/madler/zlib.git
            GIT_TAG v1.3.1
        )
        set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(zlib)
        if(TARGET zlibstatic AND NOT TARGET ZLIB::ZLIB)
            target_include_directories(zlibstatic PUBLIC "${zlib_SOURCE_DIR}" "${zlib_BINARY_DIR}")
            add_library(ZLIB::ZLIB ALIAS zlibstatic)
            set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)
            set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR};${zlib_BINARY_DIR}" CACHE PATH "" FORCE)
            set(ZLIB_INCLUDE_DIRS "${zlib_SOURCE_DIR};${zlib_BINARY_DIR}" CACHE PATH "" FORCE)
            set(ZLIB_LIBRARY zlibstatic CACHE STRING "" FORCE)
            set(ZLIB_LIBRARIES zlibstatic CACHE STRING "" FORCE)
        endif()
    endif()
endif()

# Fetch CURL if not already provided by vcpkg/system
if(NOT CURL_FOUND AND NOT TARGET CURL::libcurl AND NOT TARGET libcurl)
    FetchContent_Declare(
        curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG curl-8_7_1
    )
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    set(USE_LIBIDN2 OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)
    set(USE_NGHTTP2 OFF CACHE BOOL "" FORCE)
    set(CURL_BROTLI OFF CACHE BOOL "" FORCE)
    set(CURL_ZSTD OFF CACHE BOOL "" FORCE)
    if(WIN32)
        # Use native Windows Schannel TLS so OpenSSL is not required on Windows
        set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
        set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    endif()
    FetchContent_MakeAvailable(curl)
endif()

# Fetch Drogon (with static JsonCpp) if not already provided by vcpkg/system
if(NOT Drogon_FOUND AND NOT TARGET Drogon::Drogon AND NOT TARGET drogon)
    FetchContent_Declare(
        jsoncpp
        GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
        GIT_TAG 1.9.5
    )
    set(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
    set(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF CACHE BOOL "" FORCE)
    set(JSONCPP_WITH_CMAKE_PACKAGE OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
    set(BUILD_OBJECT_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(jsoncpp)
    file(MAKE_DIRECTORY "${jsoncpp_BINARY_DIR}/include/json")

    if(TARGET jsoncpp_static)
        set(_JSONCPP_STATIC_TARGET jsoncpp_static)
    else()
        set(_JSONCPP_STATIC_TARGET jsoncpp_lib)
    endif()
    set(Jsoncpp_FOUND TRUE CACHE BOOL "" FORCE)
    set(JSONCPP_INCLUDE_DIRS "${jsoncpp_SOURCE_DIR}/include" CACHE PATH "" FORCE)
    set(JSONCPP_LIBRARIES ${_JSONCPP_STATIC_TARGET} CACHE STRING "" FORCE)
    if(NOT TARGET Jsoncpp_lib)
        add_library(Jsoncpp_lib INTERFACE IMPORTED)
    endif()
    set_target_properties(Jsoncpp_lib PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${jsoncpp_SOURCE_DIR}/include"
        INTERFACE_LINK_LIBRARIES "${_JSONCPP_STATIC_TARGET}"
    )

    FetchContent_Declare(
        drogon
        GIT_REPOSITORY https://github.com/drogonframework/drogon.git
        GIT_TAG v1.9.8
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_CTL OFF CACHE BOOL "" FORCE)
    set(BUILD_ORM OFF CACHE BOOL "" FORCE)
    set(BUILD_POSTGRESQL OFF CACHE BOOL "" FORCE)
    set(BUILD_MYSQL OFF CACHE BOOL "" FORCE)
    set(BUILD_SQLITE OFF CACHE BOOL "" FORCE)
    set(BUILD_REDIS OFF CACHE BOOL "" FORCE)
    set(BUILD_BROTLI OFF CACHE BOOL "" FORCE)
    set(BUILD_YAML_CONFIG OFF CACHE BOOL "" FORCE)
    set(BUILD_C-ARES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(drogon)
endif()

# 2. Define ImGui Sources
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)

option(USE_TSAN "Enable ThreadSanitizer" OFF)
option(USE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# Prevent enabling both at the same time
if (USE_TSAN AND USE_ASAN)
    message(FATAL_ERROR "You cannot enable both TSan and ASan at the same time. Choose one.")
endif()

# Setup compiler and linker flags based on the selection
if ((USE_TSAN OR USE_ASAN) AND MSVC)
    message(FATAL_ERROR "Sanitizer options are only supported with GCC or Clang")
elseif (USE_TSAN)
    message(STATUS "ThreadSanitizer (TSan) Enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -g -O1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
elseif (USE_ASAN)
    message(STATUS "AddressSanitizer (ASan) and UBSan Enabled")
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
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if(WIN32)
    target_compile_definitions(sentinel PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        _WIN32_WINNT=0x0A00
    )
endif()

target_compile_options(sentinel PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/utf-8;/Zc:__cplusplus;/EHsc;/bigobj>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra>
)

# Statically link libstdc++ and libgcc on Linux / MinGW so target machines don't need a C++23 runtime installed
if((UNIX AND NOT APPLE) OR MINGW)
    target_link_options(sentinel PRIVATE -static-libstdc++ -static-libgcc)
endif()

# 4. Include Directories
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
if(TARGET Drogon::Drogon)
    set(DROGON_TARGET Drogon::Drogon)
else()
    set(DROGON_TARGET drogon)
endif()

if(TARGET CURL::libcurl)
    set(CURL_TARGET CURL::libcurl)
else()
    set(CURL_TARGET libcurl)
endif()

target_link_libraries(sentinel PRIVATE
    glfw
    OpenGL::GL
    ${DROGON_TARGET}
    Threads::Threads
    ${CURL_TARGET}
)
if(UNIX AND NOT APPLE)
    target_link_libraries(sentinel PRIVATE X11::X11)
endif()
if(WIN32)
    target_link_libraries(sentinel PRIVATE
        ws2_32
        rpcrt4
        bcrypt
        crypt32
        iphlpapi
        shlwapi
        advapi32
        user32
        gdi32
        shell32
        uuid
    )
endif()

# Keep runtime files next to the executable for single and multi-config generators.
add_custom_command(TARGET sentinel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/fonts" "$<TARGET_FILE_DIR:sentinel>/fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/sentinel.json" "$<TARGET_FILE_DIR:sentinel>/sentinel.json"
)
