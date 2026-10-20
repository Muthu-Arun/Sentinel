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
#include <drogon/HttpTypes.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cfloat>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "imgui.h"
#include "implot.h"
#include "util.h"

#define SpaceInBetweenWidgetX 14.0f
namespace Widgets {
inline ImFont* g_FontRegular = nullptr;
inline ImFont* g_FontBold = nullptr;

inline ImPlotContext* plot_context = nullptr;

void init();
void cleanup();

// Data expressed by the widget is owned by it to create a double buffer

// these buffers with atomics & mutexes will mostlikey be removed as the json parsing and updating
// will probably remain single threaded
class Widget {
public:
    std::string label;
    std::atomic<bool> is_data_available = false;
    bool isInline = false;
    float inline_width = -1.0f;
    // std::mutex data_mtx;

    Widget(std::string_view _label);

    virtual void draw() = 0;
    virtual void makeInline(float offset = 0.0f, float spacing = 0.0f);

    virtual ~Widget();
};

template <typename _data_type>
    requires /*std::integral<_data_type> || */ std::floating_point<_data_type>
class Plot : public Widget {
public:
    enum class type : uint8_t { Line, Histogram };
    std::atomic<_data_type>& src;
    type ptype;
    size_t buffer_max_limit = 16384;  // 2^14
    std::vector<_data_type> data = std::vector<_data_type>(buffer_max_limit, 0);

    Plot(std::string_view _label, type _ptype, std::atomic<_data_type>& _src)
        : Widget(_label), src(_src), ptype(_ptype) {
        pushData();
    }

    void draw() override {
        if (isInline) {
            makeInline(0.0f, ImGui::GetStyle().ItemSpacing.x + SpaceInBetweenWidgetX);
        }
        copyFromSource();

        ImGui::PushID(label.c_str());
        ImGui::BeginGroup();

        const float width = (inline_width > 0.0f) ? inline_width : ImGui::GetContentRegionAvail().x;

        // Header row with title and latest metric readout
        ImGui::TextUnformatted(label.c_str());
        const std::string val_text = std::format("{:.2f}", latest_value);
        const float val_width = ImGui::CalcTextSize(val_text.c_str()).x;
        if (width > val_width + 80.0f) {
            ImGui::SameLine(width - val_width);
            ImGui::TextColored(ImVec4(0.19f, 0.78f, 0.86f, 1.0f), "%s", val_text.c_str());
        }

        const int plot_count = static_cast<int>(count);
        const int plot_offset = (count < buffer_max_limit) ? 0 : static_cast<int>(head);

        switch (ptype) {
            case type::Line:
                if (ImPlot::BeginPlot("##LinePlot", ImVec2(inline_width, 230.0f),
                                      ImPlotFlags_NoTitle | ImPlotFlags_NoLegend |
                                          ImPlotFlags_NoMenus)) {
                    ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_AutoFit,
                                      ImPlotAxisFlags_AutoFit);
                    if (plot_count > 0) {
                        ImPlot::PlotShaded(label.c_str(), data.data(), plot_count, -INFINITY, 1.0,
                                           0.0, 0, plot_offset);
                        ImPlot::PlotLine(label.c_str(), data.data(), plot_count, 1.0, 0.0, 0,
                                         plot_offset);
                    }
                    ImPlot::EndPlot();
                }
                break;
            case type::Histogram:
                ImGui::PlotHistogram("##HistPlot", data.data(), plot_count, plot_offset, nullptr,
                                     FLT_MAX, FLT_MAX, ImVec2(inline_width, 200.0f));
                break;
        }

        ImGui::EndGroup();
        ImGui::PopID();
    }

    void copyFromSource() {
        if (is_data_available.load()) {
            pushData();
            is_data_available.store(false);
        }
    }
    ~Plot() override = default;

private:
    size_t head = 0;
    size_t count = 0;
    _data_type latest_value = 0;

    void pushData() {
        latest_value = src.load();
        if (count < buffer_max_limit) {
            data[count++] = latest_value;
        } else {
            data[head] = latest_value;
            head = (head + 1) & (buffer_max_limit - 1);
        }
    }
};

template <typename _data_type = double>
    requires std::floating_point<_data_type>
class BarPlot : public Widget {
public:
    std::vector<_data_type> data, &src, pos;
    std::vector<std::string> data_label, &src_label;
    std::vector<const char*> label_format_Implot_axis;
    int prev_bar_count = 0;
    std::mutex& src_mtx;

    BarPlot(std::string_view _label, std::vector<_data_type>& _src,
            std::vector<std::string>& _src_label, std::mutex& _src_mtx)
        : Widget(_label), src(_src), src_label(_src_label), src_mtx(_src_mtx) {
        is_data_available.store(true);
        copyFromSource();
    }

    void draw() override {
        copyFromSource();

        if (isInline) {
            makeInline(0.0f, ImGui::GetStyle().ItemSpacing.x + SpaceInBetweenWidgetX);
        }

        ImGui::PushID(label.c_str());
        ImGui::BeginGroup();

        ImGui::TextUnformatted(label.c_str());
        if (ImPlot::BeginPlot("##BarPlot", ImVec2(inline_width, 240.0f),
                              ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMenus)) {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            if (!pos.empty() && !label_format_Implot_axis.empty()) {
                const int tick_count =
                    static_cast<int>(std::min(pos.size(), label_format_Implot_axis.size()));
                ImPlot::SetupAxisTicks(ImAxis_X1, pos.data(), tick_count,
                                       label_format_Implot_axis.data());
            }
            if (!data.empty()) {
                ImPlot::PlotBars(label.c_str(), data.data(), static_cast<int>(data.size()), 0.55);
                const size_t n = std::min(pos.size(), data.size());
                for (size_t i = 0; i < n; ++i) {
                    ImPlot::Annotation(pos[i], data[i], ImVec4(0, 0, 0, 0), ImVec2(0, -10), true,
                                       "%.1f", data[i]);
                }
            }
            ImPlot::EndPlot();
        }

        ImGui::EndGroup();
        ImGui::PopID();
    }

    void copyFromSource() {
        if (is_data_available.load()) {
            std::lock_guard<std::mutex> lock(src_mtx);
            data = src;
            if (!src_label.empty()) {
                data_label = src_label;
            }
            is_data_available.store(false);
            build_label_format();
        }
    }

    void build_label_format() {
        pos.clear();
        pos.reserve(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            pos.push_back(static_cast<double>(i));
        }
        prev_bar_count = static_cast<int>(data.size());

        label_format_Implot_axis.clear();
        label_format_Implot_axis.reserve(data_label.size());
        for (std::string& elems : data_label) {
            label_format_Implot_axis.push_back(elems.c_str());
        }
    }
};

template <typename _callable =
              std::function<void(const std::string&, drogon::HttpMethod, const std::string&)>>
class Button : public Widget {
private:
    std::string endpoint, body;
    drogon::HttpMethod method;
    _callable call_on_event;

public:
    enum class event : uint8_t { ON_CLICK, ON_RELEASE };

    Button(std::string_view _label, const std::string& _endpoint, drogon::HttpMethod _method,
           std::function<void(const std::string&, drogon::HttpMethod, const std::string&)>&&
               _call_on_event)
        : Widget(_label),
          endpoint(_endpoint),
          method(_method),
          call_on_event(std::move(_call_on_event)) {}

    void draw() override {
        if (isInline) {
            makeInline(0.0f, ImGui::GetStyle().ItemSpacing.x + SpaceInBetweenWidgetX);
        }
        ImGui::PushID(label.c_str());
        ImGui::BeginGroup();
        const float btn_width = (inline_width > 0.0f) ? inline_width : 0.0f;
        if (ImGui::Button(label.c_str(), ImVec2(btn_width, 34.0f))) {
            if (call_on_event) {
                call_on_event(endpoint, method, body);
            }
        }
        ImGui::EndGroup();
        ImGui::PopID();
    }
    ~Button() override = default;
};

template <typename _data_type = std::string>
class Text : public Widget {
public:
    _data_type data;

    const _data_type& src;
    std::mutex& src_mtx;

    Text(std::string_view _label, const _data_type& src, std::mutex& src_mtx)
        : Widget(_label), data(src), src(src), src_mtx(src_mtx) {
        is_data_available.store(false);
    }

    void draw() override {
        copyFromSource();
        if (isInline) {
            makeInline(0.0f, ImGui::GetStyle().ItemSpacing.x + SpaceInBetweenWidgetX);
        }
        ImGui::PushID(label.c_str());
        ImGui::BeginGroup();

        const float card_w =
            (inline_width > 0.0f) ? inline_width : ImGui::GetContentRegionAvail().x;
        const float pad_x = 12.0f;
        const float pad_y = 8.0f;
        const float wrap_w = std::max(60.0f, card_w - pad_x * 2.0f - 4.0f);

        const ImVec2 label_sz = ImGui::CalcTextSize(label.c_str());
        const ImVec2 text_sz =
            ImGui::CalcTextSize(data.c_str(), nullptr, false, wrap_w);
        const float card_h = pad_y * 2.0f + label_sz.y + 4.0f + text_sz.y;

        const ImVec2 p0 = ImGui::GetCursorScreenPos();
        const ImVec2 p1(p0.x + card_w, p0.y + card_h);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p0, p1, ImGui::GetColorU32(ImGuiCol_ChildBg), 6.0f);
        dl->AddRect(p0, p1, ImGui::GetColorU32(ImGuiCol_Border), 6.0f);
        dl->AddRectFilled(p0, ImVec2(p0.x + 3.5f, p1.y),
                          ImGui::GetColorU32(ImGuiCol_PlotHistogram), 6.0f,
                          ImDrawFlags_RoundCornersLeft);

        ImGui::Dummy(ImVec2(card_w, card_h));

        const ImVec2 label_pos(p0.x + pad_x + 2.0f, p0.y + pad_y);
        dl->AddText(label_pos, ImGui::GetColorU32(ImGuiCol_TextDisabled), label.c_str());

        const ImVec2 val_pos(p0.x + pad_x + 2.0f, p0.y + pad_y + label_sz.y + 4.0f);
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), val_pos,
                    ImGui::GetColorU32(ImGuiCol_Text), data.c_str(), nullptr, wrap_w);

        ImGui::EndGroup();
        ImGui::PopID();
    }

    void copyFromSource() {
        if (is_data_available.load()) {
            std::lock_guard<std::mutex> lock(src_mtx);
            data = src;
            is_data_available.store(false);
        }
    }
    ~Text() override = default;
};

template <typename _data_type = float>
    requires std::integral<_data_type> || std::floating_point<_data_type>
class RadialGauge : public Widget {
protected:
    struct Coordinates {
        float width;
        float radius;
        float start_angle;
        float end_angle;
    };

public:
    std::array<_data_type, 2> range;
    std::atomic<_data_type>& data;
    Coordinates coordinates;

    RadialGauge(std::string_view _label, _data_type min, _data_type max,
                std::atomic<_data_type>& _data,
                Coordinates coordn = {196.0f, 196.0f / 2.0f, 3.14159265f * 0.75f,
                                      3.14159265f * 2.25f})
        : Widget(_label), range{min, max}, data(_data), coordinates(coordn) {}

    void draw() override {
        if (isInline) {
            makeInline(0.0f, ImGui::GetStyle().ItemSpacing.x + SpaceInBetweenWidgetX);
        }
        ImGui::PushID(label.c_str());
        ImGui::BeginGroup();

        const float box_w =
            (inline_width > 0.0f) ? std::max(coordinates.width, inline_width) : coordinates.width;
        const float box_h = coordinates.width;
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 box_max(pos.x + box_w, pos.y + box_h);
        const ImVec2 center(pos.x + box_w * 0.5f, pos.y + box_h * 0.52f);
        ImGui::Dummy(ImVec2(box_w, box_h));

        const _data_type temp_data = data.load();
        const float min_val = static_cast<float>(range[0]);
        const float max_val = static_cast<float>(range[1]);
        const float span = max_val - min_val;
        const float normalized =
            (std::abs(span) > 1e-6f)
                ? std::clamp((static_cast<float>(temp_data) - min_val) / span, 0.0f, 1.0f)
                : 0.0f;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Subtle card background
        draw_list->AddRectFilled(pos, box_max, ImGui::GetColorU32(ImGuiCol_ChildBg), 8.0f);
        draw_list->AddRect(pos, box_max, ImGui::GetColorU32(ImGuiCol_Border), 8.0f);

        const float thickness = 11.0f;
        const float arc_radius = coordinates.radius - thickness - 12.0f;

        // Background track arc
        draw_list->PathArcTo(center, arc_radius, coordinates.start_angle, coordinates.end_angle,
                             48);
        draw_list->PathStroke(ImGui::GetColorU32(ImGuiCol_FrameBg), 0, thickness);

        // Semantic color based on threshold
        ImU32 accent = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
        if (normalized >= 0.90f) {
            accent = IM_COL32(239, 83, 80, 255);  // Coral red alert
        } else if (normalized >= 0.75f) {
            accent = IM_COL32(255, 179, 0, 255);  // Amber warning
        }

        // Foreground active arc
        if (normalized > 0.002f) {
            const float current_angle =
                coordinates.start_angle +
                (coordinates.end_angle - coordinates.start_angle) * normalized;
            draw_list->PathArcTo(center, arc_radius, coordinates.start_angle, current_angle, 48);
            draw_list->PathStroke(accent, 0, thickness);
        }

        // Format numeric value cleanly
        std::string val_str;
        if constexpr (std::floating_point<_data_type>) {
            val_str = std::format("{:.1f}", temp_data);
        } else {
            val_str = std::format("{}", temp_data);
        }

        ImFont* bold_font = g_FontBold ? g_FontBold : ImGui::GetFont();
        ImFont* reg_font = g_FontRegular ? g_FontRegular : ImGui::GetFont();

        const ImVec2 value_size = bold_font->CalcTextSizeA(24.0f, FLT_MAX, 0.0f, val_str.c_str());
        const ImVec2 label_size = reg_font->CalcTextSizeA(14.5f, FLT_MAX, 0.0f, label.c_str());

        const ImVec2 val_pos(center.x - value_size.x * 0.5f, center.y - value_size.y * 0.5f - 6.0f);
        const ImVec2 label_pos(center.x - label_size.x * 0.5f, center.y + 12.0f);

        draw_list->AddText(bold_font, 24.0f, val_pos, accent, val_str.c_str());
        draw_list->AddText(reg_font, 14.5f, label_pos, ImGui::GetColorU32(ImGuiCol_Text),
                           label.c_str());

        // Min / Max range tick labels
        const std::string min_str = std::format("{:.0f}", min_val);
        const std::string max_str = std::format("{:.0f}", max_val);
        const ImVec2 min_sz = reg_font->CalcTextSizeA(11.5f, FLT_MAX, 0.0f, min_str.c_str());
        const ImVec2 max_sz = reg_font->CalcTextSizeA(11.5f, FLT_MAX, 0.0f, max_str.c_str());

        const ImVec2 min_pt(center.x + std::cos(coordinates.start_angle) * (arc_radius - 2.0f) -
                                min_sz.x * 0.5f,
                            center.y + std::sin(coordinates.start_angle) * (arc_radius - 2.0f) +
                                8.0f);
        const ImVec2 max_pt(center.x + std::cos(coordinates.end_angle) * (arc_radius - 2.0f) -
                                max_sz.x * 0.5f,
                            center.y + std::sin(coordinates.end_angle) * (arc_radius - 2.0f) +
                                8.0f);
        draw_list->AddText(reg_font, 11.5f, min_pt, ImGui::GetColorU32(ImGuiCol_TextDisabled),
                           min_str.c_str());
        draw_list->AddText(reg_font, 11.5f, max_pt, ImGui::GetColorU32(ImGuiCol_TextDisabled),
                           max_str.c_str());

        ImGui::EndGroup();
        ImGui::PopID();
    }

    ~RadialGauge() override = default;
};

class TextInput : public Widget {
public:
    const std::string& src;
    std::mutex& src_mtx;
    size_t string_capacity;  // Declare before string, using memb init list
    std::string data;

    TextInput(std::string_view label, const std::string& src, std::mutex& src_mtx,
              size_t string_capacity = 1024);
    ~TextInput() override;
    void draw() override;
    void copyFromSource();
};

template <typename _data_type = std::string>
class Image : public Widget {
public:
    std::string data;  // Request for the Image with its label from the provided endpoint
    std::string endpoint;
    const std::string& src;
    std::mutex& src_mtx;
    std::expected<GLuint, std::string> texture = std::unexpected("Not loaded");
    ImVec2 img_size{0.0f, 0.0f};

    Image(std::string_view _label, const std::string& _src, std::mutex& _src_mtx,
          const std::string& _endpoint)
        : Widget(_label), endpoint(_endpoint), src(_src), src_mtx(_src_mtx) {}

    void draw() override {
        if (isInline) {
            makeInline(0.0f, ImGui::GetStyle().ItemSpacing.x + SpaceInBetweenWidgetX);
        }
        copyFromSource();

        ImGui::PushID(label.c_str());
        ImGui::BeginGroup();
        ImGui::TextUnformatted(label.c_str());
        if (!data.empty() && texture.has_value()) [[likely]] {
            const float max_w =
                (inline_width > 0.0f) ? inline_width : ImGui::GetContentRegionAvail().x;
            ImVec2 display_size = img_size;
            if (display_size.x > max_w && display_size.x > 0.0f && max_w > 0.0f) {
                const float scale = max_w / display_size.x;
                display_size.x = max_w;
                display_size.y *= scale;
            }
            ImGui::Image(static_cast<ImTextureID>(texture.value()), display_size);
        } else {
            const float box_w =
                (inline_width > 0.0f) ? inline_width : ImGui::GetContentRegionAvail().x;
            const ImVec2 p0 = ImGui::GetCursorScreenPos();
            const ImVec2 p1(p0.x + std::max(120.0f, box_w), p0.y + 90.0f);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(p0, p1, ImGui::GetColorU32(ImGuiCol_ChildBg), 6.0f);
            dl->AddRect(p0, p1, ImGui::GetColorU32(ImGuiCol_Border), 6.0f);
            dl->AddText(ImVec2(p0.x + 12.0f, p0.y + 36.0f),
                        ImGui::GetColorU32(ImGuiCol_TextDisabled), "Loading image...");
            ImGui::Dummy(ImVec2(std::max(120.0f, box_w), 90.0f));
        }
        ImGui::EndGroup();
        ImGui::PopID();
    }

    void copyFromSource() {
        if (is_data_available.load()) {
            {
                std::lock_guard<std::mutex> lock(src_mtx);
                data = src;
                is_data_available.store(false);
            }
            if (texture.has_value()) {
                GLuint tex_id = texture.value();
                glDeleteTextures(1, &tex_id);
            }
            texture = Utils::Image::genTextureFromImageBuffer(data, img_size);
        }
    }

    ~Image() override {
        if (texture.has_value()) {
            GLuint tex_id = texture.value();
            glDeleteTextures(1, &tex_id);
        }
    }
};

class Table : public Widget {
public:
    using tableRowContainerElem = std::variant<int, float, std::string>;
    using tableRowContainer = std::vector<tableRowContainerElem>;
    int ncol = 0;

protected:
    // Preserve the column types for future use
    std::vector<std::string> header;
    std::vector<tableRowContainer>& src;
    std::mutex& src_mtx;
    std::vector<tableRowContainer> rows;

public:
    Table(std::string_view _label, std::vector<std::string>&& _header,
          std::vector<tableRowContainer>& _src, std::mutex& _src_mtx);
    void draw() override;
    void copyFromSource();
};
}  // namespace Widgets
