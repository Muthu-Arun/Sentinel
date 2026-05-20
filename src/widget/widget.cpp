#include "widget.h"

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

#include "imgui.h"
#include "implot.h"

namespace Widgets {
void init() {
    plot_context = ImPlot::CreateContext();
}
void cleanup() {
    ImPlot::DestroyContext(plot_context);
}
Widget::Widget(std::string_view _label) : label(_label) {}
Widget::~Widget() {}
// FIX textinput so the dest is the src, and modify the mutexes accordingly
TextInput::TextInput(std::string_view label, const std::string& src, std::mutex& src_mtx,
                     size_t string_capacity)
    : Widget(label),
      src(src),
      src_mtx(src_mtx),
      string_capacity(string_capacity),
      data(string_capacity, ' ') {}

TextInput::~TextInput() {}

void Widget::makeInline(float offset, float spacing) {
    ImGui::SameLine(offset, spacing);
}

void TextInput::draw() {
    copyFromSource();
    ImGui::InputText(label.c_str(), data.data(), string_capacity);
}

void TextInput::copyFromSource() {
    if (is_data_available.load()) {
        std::lock_guard<std::mutex> lock(src_mtx);
        data = src;
        is_data_available.store(false);
    }
}
}  // namespace Widgets