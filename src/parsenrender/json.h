#pragma once
#include <drogon/HttpTypes.h>
#include <json/value.h>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "poll.h"
#include "widget.h"
#include "window.h"
namespace ParseJson {

class HttpWindowWrapper {
protected:
    std::unordered_map<std::string, std::atomic<int>> map_int;
    std::unordered_map<std::string, std::atomic<float>> map_float;
    std::unordered_map<std::string, std::string> map_string;
    std::unordered_map<std::string, std::mutex> network_buffer_mtx;
    std::unordered_map<std::string, std::function<void(const std::string& id, const Json::Value&)>>
        widget_updates_fr;
    std::unordered_map<std::string, std::vector<double>> map_vector_double;
    std::unordered_map<std::string, std::vector<std::string>> map_vector_string;
    std::array<char, 250> host{}, host_endpoint{};
    uint64_t port = 80;
    bool in_init_phase;

public:
    static uint32_t window_count;
    uint32_t win_idx;
    std::string win_label;
    HttpWindowWrapper();
    HttpWindowWrapper(const HttpWindowWrapper&) = delete;
    // HttpWindowWrapper(HttpWindowWrapper&&) = default;
    HttpWindowWrapper(const std::string& label, const std::string& host,
                      const std::string& endpoint, const int port);
    std::optional<HttpPoll::Poll> poll;
    std::optional<Window::Window> window;

    void addText(const std::string& _label, std::string_view data);
    // void addRadialGauge(const std::string& _label, int data, int min, int max);
    void addRadialGauge(const std::string& _label, float data, float min, float max);
    void addPlot(const std::string& _label, float data,
                 Widgets::Plot<float>::type ptype = Widgets::Plot<float>::type::Line);

    void addBarPlot(const std::string& _label, const std::vector<double>& data,
                    const std::vector<std::string>& format_labels);

    void addButton(const std::string& _label, const std::string& endpoint,
                   drogon::HttpMethod method);

    void addImage(const std::string& _label, const std::string& endpoint);

    void renderHeader();
    void parseJSON();
    void initFRs();
};
}  // namespace ParseJson
