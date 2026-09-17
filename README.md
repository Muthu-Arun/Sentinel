# Sentinel

![Language](https://img.shields.io/badge/language-C%2B%2B23-blue.svg)
![UI](https://img.shields.io/badge/UI-Dear%20ImGui-orange.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

**A High-Performance, Server-Driven UI (SDUI) Engine for Remote Machine Management.**

Sentinel is a native C++ application designed to decouple the user interface from the underlying logic. By parsing JSON configurations at runtime, it allows remote servers (e.g., Python, Go, or C++ backends) to define and update the client's UI dynamically. This enables "building UIs on the fly" without recompiling the client, making it perfect for monitoring remote machines, streaming data, and rapid prototyping of internal tools.

## Key Features

* **Server-Driven UI (SDUI):** The client is a stateless renderer; the server is the brain. Change your UI layout, widgets, and data binding server-side via JSON, and the client updates instantly.
* **High-Performance Native App:** Built with **Modern C++ (C++23)**, **Dear imgui** and **Drogon** ensuring low memory footprint and immediate-mode rendering speeds that web dashboards can't match.
* **Asynchronous Networking:** Non-blocking I/O ensures the UI remains responsive even while streaming heavy data payloads or handling complex network requests.
* **Hot-Reloading:** Modify the JSON response on your server, and Sentinel reflects the changes in real-time on the next fetch cycle.
* **Cross-Platform:** Builds on Linux, Windows, and macOS.

## Tech Stack

* **Core:** C++23
* **UI Library:** [Dear ImGui](https://github.com/ocornut/imgui) (Immediate Mode GUI)
* **Networking:** [Drogon](https://github.com/drogonframework/drogon)
* **Build System:** CMake

## Architecture

Sentinel follows a strict separation of concerns:

1.  **Network Layer:** Polls or streams configuration data from a specified endpoint.
2.  **State Layer:** Parses JSON into a local state object using double-buffering to prevent read/write tearing.
3.  **Render Layer:** The ImGui loop reads the "active" buffer and reconstructs the UI every frame based on the schema definitions (e.g., `{ "type": "plot", "data": [...] }`).

## Build Instructions

### Prerequisites

* A C++23 compiler: GCC, Clang, Apple Clang, or a current Visual Studio 2022
* CMake 3.20 or newer
* OpenGL, GLFW, Drogon, and libcurl

### Linux (Ubuntu)

```bash
sudo apt update
sudo apt install libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev
sudo apt install libstdc++-14-dev clang-20

# Install Drogon
git clone https://github.com/drogonframework/drogon.git
sudo apt install libjsoncpp-dev uuid-dev zlib1g-dev libssl-dev
sudo apt install libcurl4-openssl-dev libcurl4
cd drogon && sudo ./build.sh && cd ..

cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++-20
cmake --build build
```

### macOS

Install Xcode Command Line Tools and the dependencies with Homebrew:

```bash
xcode-select --install
brew install cmake curl drogon glfw
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake --build build
```

### Windows

Install Visual Studio 2022 with the **Desktop development with C++** workload, CMake, and
[vcpkg](https://learn.microsoft.com/vcpkg/get_started/get-started). The included manifest
installs the required libraries:

```powershell
cmake -S . -B build -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

The executable and its required `fonts` and `sentinel.json` runtime files are placed together
in the selected build configuration's output directory.

### Portable vcpkg Build

The same manifest can be used on Linux and macOS by supplying the vcpkg toolchain:

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```
