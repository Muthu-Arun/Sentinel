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
Table::Table(std::string_view _label, std::vector<std::string>&& _header,
             std::vector<tableRowContainer>& _src, std::mutex& _src_mtx)
    : Widget(_label), header(std::move(_header)), src(_src), src_mtx(_src_mtx) {
    ncol = header.size();
    rows.emplace_back();
}
void Table::copyFromSource() {
    std::lock_guard<std::mutex> _lock(src_mtx);
    // rows.emplace_back(src);
    for (const auto& row : src) {
        rows.emplace_back(row);
    }
    src.clear();
}
void Table::draw() {
    if (is_data_available.load()) {
        copyFromSource();
    }
    if (!ImGui::BeginTable(label.c_str(), ncol)) [[unlikely]] {
        return;
    }
    for (const std::string& hc : header) {
        ImGui::TableSetupColumn(hc.c_str());
    }
    ImGui::TableHeadersRow();
    for (const auto& row : rows) {
        ImGui::TableNextRow();
        for (const auto& col : row) {
            std::visit(
                [](const auto& val) {
                    using type = std::decay_t<decltype(val)>;
                    ImGui::TableNextColumn();
                    if constexpr (std::is_same_v<type, int>) {
                        ImGui::Text("%d", val);
                    } else if constexpr (std::is_same_v<type, float>) {
                        ImGui::Text("%f", val);
                    } else if constexpr (std::is_same_v<type, std::string>) {
                        ImGui::Text("%s", val.c_str());
                    }
                },
                col);
        }
    }
    ImGui::EndTable();
}
}  // namespace Widgets