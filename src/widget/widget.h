#pragma once
#include <GL/gl.h>
#include <drogon/HttpTypes.h>

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
#include <vector>

#include "imgui.h"
#include "implot.h"
#include "util.h"
namespace Widgets {
inline ImFont* g_FontRegular = nullptr;
inline ImFont* g_FontBold = nullptr;

inline ImPlotContext* plot_context;

void init();
void cleanup();

// Data expressed by the widget is owned by it to create a double buffer

// these buffers with atomics & mutexes will mostlikey be removed as the json parsing and updating will probably remain
// single threaded
class Widget {
public:
    std::string label;
    // std::mutex data_mtx;
    std::atomic<bool> is_data_available = 0;
    Widget(std::string_view _label);
    virtual void draw() = 0;
    // virtual void action();
    // virtual void copyFromSource() = 0;
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
        : Widget(_label), src(_src), ptype(_ptype) {}

    void draw() override {
        // TODO
        // float window_width = ImGui::GetWindowWidth();
        // float window_height = ImGui::GetWindowHeight();
        copyFromSource();
        switch (ptype) {
            case type::Line:
                // ImGui::PlotLines(label.c_str(), data.data(), buffer_max_limit, head, NULL,
                // FLT_MAX, FLT_MAX, ImVec2(window_width, window_height));
                if (ImPlot::BeginPlot(label.c_str())) {
                    ImPlot::PlotLine(label.c_str(), data.data(), buffer_max_limit, 1, 0, 0, head);
                    ImPlot::EndPlot();
                }
                break;
            case type::Histogram:
                ImGui::PlotHistogram(label.c_str(), data.data(), buffer_max_limit, head);
                break;
        }
    }

    void copyFromSource() {
        if (is_data_available.load()) {
            pushData();
            is_data_available.store(false);
        }
    }
    ~Plot() {}

private:
    size_t head = 0;
    void pushData() {
        data[head] = src.load();
        head = (head + 1) & (buffer_max_limit - 1);
    }
};
// TODO - After implementing bar plots
/*
template <typename _data_type>  //, size_t buffer_size = 16384> // 16384 is 2^14
class Histogram : public Widget {
public:
    std::vector<_data_type> data;
    std::vector<_data_type>& src;
    std::mutex& src_mtx;
    Histogram(std::string_view _label, std::vector<_data_type>& _src, std::mutex& _src_mtx)
        : Widget(_label), src(_src), src_mtx(_src_mtx) {};
    void draw() override {
        ImPlot::BeginPlot(label.c_str());
        ImPlot::PlotHistogram(label.c_str(), data.data(), data.size());
        ImPlot::EndPlot();
    }
    void copyFromSource() {
        if (is_data_available.load()) {
            std::lock_guard<std::mutex> lock(src_mtx);
            data = src;
            is_data_available.load(false);
        }
    }
};
*/
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
        : Widget(_label), src(_src), src_label(_src_label), src_mtx(_src_mtx) {}
    void draw() override {
        copyFromSource();
        if (ImPlot::BeginPlot(label.c_str())) {
            // ImPlot::SetupAxes("Category", "Value", ImPlotAxisFlags_AutoFit,
            // ImPlotAxisFlags_AutoFit); Utils::Log::logVec(src_label);
            ImPlot::SetupAxisTicks(ImAxis_X1, pos.data(), label_format_Implot_axis.size(),
                                   label_format_Implot_axis.data());
            ImPlot::PlotBars(label.c_str(), data.data(), data.size());
            // Utils::Log::logVec(data);
            for (int i = 0; i < pos.size(); i++) {
                ImPlot::Annotation(pos[i], data[i], ImVec4(0, 0, 0, 0), ImVec2(0, -10), true,
                                   "%.1f", data[i]);
            }
            ImPlot::EndPlot();
        }
    }
    void copyFromSource() {
        if (is_data_available.load()) {
            std::lock_guard<std::mutex> lock(src_mtx);
            // std::cerr << "Logging form CPS\n";
            // Utils::Log::logVec(src);
            data = src;
            data_label = src_label;
            is_data_available.store(false);
            build_label_format();
        }
    }
    void build_label_format() {
        if (data.size() != prev_bar_count) {
            for (int i = 0; i < data.size(); i++) {
                pos.push_back(static_cast<double>(i));
            }
            prev_bar_count = data.size();
        }
        label_format_Implot_axis.clear();
        for (std::string& elems : data_label) {
            label_format_Implot_axis.push_back(elems.data());
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
    std::future<void> btn_ftr;

public:
    enum class event : uint8_t { ON_CLICK, ON_RELEASE };

    Button(std::string_view _label, const std::string& _endpoint, drogon::HttpMethod _method,
           std::function<void(const std::string&, drogon::HttpMethod, const std::string&)>&&
               _call_on_event)
        : Widget(_label), endpoint(_endpoint), method(_method), call_on_event(_call_on_event) {}

    void draw() override {
        if (ImGui::Button(label.c_str())) {
            std::cerr << "Executing Button Event\n";
            // call_on_event(endpoint, drogon::HttpMethod::Get, body);
            btn_ftr = std::async(std::launch::async, call_on_event, endpoint,
                                 drogon::HttpMethod::Get, body);
            std::cerr << "Exiting Button Event\n";
        }
    }
    ~Button() {}
};

template <typename _data_type = std::string>
class Text : public Widget {
public:
    _data_type data;

    const _data_type& src;
    std::mutex& src_mtx;

    Text(std::string_view _label, const _data_type& src, std::mutex& src_mtx)
        : Widget(_label), src(src), src_mtx(src_mtx) {}
    void draw() override {
        copyFromSource();
        ImGui::Text("%s", data.c_str());
    }
    void copyFromSource() {
        if (is_data_available.load()) {
            std::lock_guard<std::mutex> lock(src_mtx);
            data = src;
            is_data_available.store(false);
        }
    }
    ~Text() {}
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
                Coordinates coordn = {200.0f, 200.0f / 2.0f, 3.14159f * 0.75f, 3.14159f * 2.25f})
        : Widget(_label), range{min, max}, data(_data), coordinates(coordn) {}

    void draw() override {
        // INIT
        ImVec2 pos = ImGui::GetCursorScreenPos();  // Top-left corner of the widget
        ImVec2 center = ImVec2(pos.x + coordinates.radius, pos.y + coordinates.radius);
        ImGui::Dummy(ImVec2(coordinates.width, coordinates.width));  // Reserve the space
        _data_type temp_data = data.load();
        // DRAW
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Draw the grey "background" arc
        draw_list->PathArcTo(center, coordinates.radius, coordinates.start_angle,
                             coordinates.end_angle,
                             32);  // 32 segments for smoothness
        draw_list->PathStroke(ImGui::GetColorU32(ImGuiCol_FrameBg), 0,
                              10.0f);  // 10px thickness
        float current_angle =
            coordinates.start_angle +
            (coordinates.end_angle - coordinates.start_angle) * (temp_data / range[1]);

        auto val_pos = ImVec2(pos.x + coordinates.width * 0.4, pos.y + coordinates.width * 0.75);
        auto label_pos = ImVec2(pos.x + 0.4 * coordinates.width, pos.y + coordinates.width * 0.6);
        draw_list->PathArcTo(center, coordinates.radius, coordinates.start_angle, current_angle,
                             32);
        draw_list->PathStroke(ImGui::GetColorU32(ImGuiCol_PlotHistogram), 0, 10.0f);
        const std::string val_str = std::format("{}", temp_data);
        // draw_list->AddText(pos, IM_COL32(255, 0, 0, 255), label.c_str());
        draw_list->AddText(g_FontBold, 26.0, label_pos, IM_COL32(255, 0, 0, 255), label.c_str());
        draw_list->AddText(val_pos, IM_COL32(0, 255, 0, 255), val_str.c_str());
        // draw_list->AddText(g_FontBold, 16.0, val_pos, IM_COL32(255, 0, 0, 255), val_str.c_str());
    }

    ~RadialGauge() {}
};
class TextInput : public Widget {
public:
    const std::string& src;
    std::mutex& src_mtx;
    size_t string_capacity;  // Declare before string, using memb init list
    std::string data;

    TextInput(std::string_view label, const std::string& src, std::mutex& src_mtx,
              size_t string_capacity = 1024);
    ~TextInput();
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
    std::expected<GLuint, std::string> texture;
    ImVec2 img_size;

    Image(std::string_view _label, const std::string& _src, std::mutex& _src_mtx,
          const std::string& _endpoint)
        : Widget(_label), endpoint(_endpoint), src(_src), src_mtx(_src_mtx) {}

    void draw() override {
        copyFromSource();
        if (data.size() && texture) [[likely]] {
            if (texture) [[likely]] {
                ImGui::Image(texture.value(), img_size);
            }
        }
    }

    void copyFromSource() {
        if (is_data_available.load()) {
            {
                std::lock_guard<std::mutex> lock(src_mtx);
                data = src;
                is_data_available.store(false);
            }
            if (texture.has_value()) {
                glDeleteTextures(1, &texture.value());
            }
            texture = Utils::Image::genTextureFromImageBuffer(data, img_size);
        }
    }
};
}  // namespace Widgets
