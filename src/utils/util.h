#pragma once
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <GL/gl.h>
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#include <cstddef>
#include <expected>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <span>

#include "imgui.h"
#include "stb_image.h"
namespace Utils {
namespace Log {
template <typename t>
void logVec(const std::vector<t>& vec) {
    std::cerr << std::format("Vector Size : {}", vec.size());
    std::cin.get();
    for (const t& elem : vec) {
        std::cerr << elem;
    }
    std::cerr << '\n';
}
}  // namespace Log
namespace Image {
std::expected<GLuint, std::string> genTextureFromImageBuffer(std::span<char> img_buf,
                                                             ImVec2& img_size);

// Returns true on success, outputs the OpenGL texture ID and dimensions
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width,
                         int* out_height);
}  // namespace Image
}  // namespace Utils
