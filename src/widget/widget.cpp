#include "widget.h"

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

#include "imgui.h"
#include "implot.h"

namespace Widgets {
namespace {
void applyVisualStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(18.0f, 16.0f);
    style.FramePadding = ImVec2(10.0f, 7.0f);
    style.CellPadding = ImVec2(10.0f, 8.0f);
    style.ItemSpacing = ImVec2(10.0f, 10.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.ScrollbarSize = 13.0f;
    style.GrabMinSize = 10.0f;

    style.WindowRounding = 10.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    const ImVec4 background(0.035f, 0.051f, 0.075f, 1.0f);
    const ImVec4 surface(0.055f, 0.078f, 0.110f, 1.0f);
    const ImVec4 raised(0.075f, 0.105f, 0.145f, 1.0f);
    const ImVec4 border(0.145f, 0.190f, 0.245f, 1.0f);
    const ImVec4 text(0.890f, 0.925f, 0.960f, 1.0f);
    const ImVec4 muted(0.515f, 0.585f, 0.665f, 1.0f);
    const ImVec4 accent(0.145f, 0.690f, 0.780f, 1.0f);
    const ImVec4 accent_hover(0.190f, 0.775f, 0.850f, 1.0f);
    const ImVec4 accent_active(0.100f, 0.585f, 0.690f, 1.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = muted;
    colors[ImGuiCol_WindowBg] = background;
    colors[ImGuiCol_ChildBg] = surface;
    colors[ImGuiCol_PopupBg] = surface;
    colors[ImGuiCol_Border] = border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_FrameBg] = raised;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.095f, 0.145f, 0.190f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.110f, 0.175f, 0.225f, 1.0f);
    colors[ImGuiCol_TitleBg] = surface;
    colors[ImGuiCol_TitleBgActive] = raised;
    colors[ImGuiCol_TitleBgCollapsed] = surface;
    colors[ImGuiCol_MenuBarBg] = surface;
    colors[ImGuiCol_ScrollbarBg] = background;
    colors[ImGuiCol_ScrollbarGrab] = border;
    colors[ImGuiCol_ScrollbarGrabHovered] = muted;
    colors[ImGuiCol_ScrollbarGrabActive] = accent;
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accent_hover;
    colors[ImGuiCol_Button] = accent_active;
    colors[ImGuiCol_ButtonHovered] = accent;
    colors[ImGuiCol_ButtonActive] = accent_hover;
    colors[ImGuiCol_Header] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.55f);
    colors[ImGuiCol_HeaderActive] = ImVec4(accent.x, accent.y, accent.z, 0.75f);
    colors[ImGuiCol_Separator] = border;
    colors[ImGuiCol_SeparatorHovered] = accent;
    colors[ImGuiCol_SeparatorActive] = accent_hover;
    colors[ImGuiCol_ResizeGrip] = ImVec4(accent.x, accent.y, accent.z, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(accent.x, accent.y, accent.z, 0.65f);
    colors[ImGuiCol_ResizeGripActive] = accent;
    colors[ImGuiCol_Tab] = raised;
    colors[ImGuiCol_TabHovered] = accent;
    colors[ImGuiCol_TabActive] = accent_active;
    colors[ImGuiCol_PlotLines] = accent_hover;
    colors[ImGuiCol_PlotHistogram] = accent;
    colors[ImGuiCol_TableHeaderBg] = raised;
    colors[ImGuiCol_TableBorderStrong] = border;
    colors[ImGuiCol_TableBorderLight] = ImVec4(border.x, border.y, border.z, 0.55f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.025f);
    colors[ImGuiCol_NavHighlight] = accent;
}
}  // namespace

void init() {
    plot_context = ImPlot::CreateContext();
    applyVisualStyle();

    ImPlotStyle& plot_style = ImPlot::GetStyle();
    plot_style.PlotPadding = ImVec2(12.0f, 12.0f);
    plot_style.LabelPadding = ImVec2(8.0f, 8.0f);
    plot_style.LegendPadding = ImVec2(10.0f, 8.0f);
    plot_style.LegendInnerPadding = ImVec2(7.0f, 5.0f);
    plot_style.LineWeight = 2.0f;
    plot_style.FillAlpha = 0.35f;
    ImPlot::StyleColorsDark();
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
    ImGui::SetNextItemWidth(-FLT_MIN);
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
        is_data_available.store(false);
    }
    constexpr ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerH |
                                      ImGuiTableFlags_RowBg |
                                      ImGuiTableFlags_Resizable |
                                      ImGuiTableFlags_SizingStretchProp |
                                      ImGuiTableFlags_ScrollY;
    if (!ImGui::BeginTable(label.c_str(), ncol, flags, ImVec2(0.0f, 0.0f))) [[unlikely]] {
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
