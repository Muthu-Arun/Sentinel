#include "json.h"

#include <drogon/HttpTypes.h>
#include <json/value.h>

#include <algorithm>
#include <atomic>
#include <exception>
#include <format>
#include <memory>
#include <mutex>
#include <print>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "imgui.h"
#include "poll.h"
#include "widget.h"
#include "window.h"
namespace ParseJson {
enum class WidgetType : uint16_t { Unknown, Plot, Histogram, Text, Image, RadialGauge };
/*
void parseDynamicJson(const Json::Value& root) {
    if (root.isObject()) {
        // It's a map/dictionary: { "key": "value" }
        for (auto const& id : root.getMemberNames()) {
            std::cout << "Key: " << id << " -> ";
            parseDynamicJson(root[id]);  // Recurse!
        }
    } else if (root.isArray()) {
        // It's a list: [1, 2, 3]
        for (unsigned int i = 0; i < root.size(); i++) {
            parseDynamicJson(root[i]);  // Recurse!
        }
    } else {
        // It's a "leaf" (Value): string, int, bool
        std::cout << "Value: " << root.asString() << std::endl;
    }
}
*/
/*
JSON Structure
{
    widget_label1: {type: widget_type, value: data},

    widget_label2: {type: widget_type, value: data},

    ....
}
*/
/*
Widget Specific JSON Structure
Text:
    type = text
    data = data_text

Plot:
    type = plot
    data = floating point or integer value

RadialGauge:
    type = radial_gauge
    data = floating point value
    min = float
    max = float

BarPlot:
    type = bar_plot
    data = array of doubles
    data_labels = array of strings
Table:
    type = table
    data = array of arrays subarrays matching the row width
    header = array of string (header names)
*/

void HttpWindowWrapper::addText(const std::string& _label, std::string_view data) {
    buffer_container[_label].emplace<std::string>(data);
    // map_string[_label] = data;  // IG using [] oprator is not a problem here as if there aren't
    // any we need to create one
    network_buffer_mtx[_label];
    window->addWidget(_label, std::make_unique<Widgets::Text<>>(
                                  _label, std::get<std::string>(buffer_container[_label]),
                                  network_buffer_mtx[_label]));
}
/*
void HttpWindowWrapper::addRadialGauge(const std::string& _label, int data, int min,
                                       int max) {  // NOT TO BE USED, TODO REMOVE
    map_int[_label] = data;
    window->addWidget(
        _label, std::make_unique<Widgets::RadialGauge<int>>(_label, min, max, map_int.at(_label)));
}
*/
void HttpWindowWrapper::addRadialGauge(const std::string& _label, float data, float min,
                                       float max) {
    buffer_container[_label].emplace<std::atomic<float>>(data);
    window->addWidget(
        _label, std::make_unique<Widgets::RadialGauge<float>>(
                    _label, min, max, std::get<std::atomic<float>>(buffer_container[_label])));
}
void HttpWindowWrapper::addPlot(const std::string& _label, float data,
                                Widgets::Plot<float>::type ptype) {
    buffer_container[_label].emplace<std::atomic<float>>(data);
    window->addWidget(_label,
                      std::make_unique<Widgets::Plot<float>>(
                          _label, ptype, std::get<std::atomic<float>>(buffer_container[_label])));
}
void HttpWindowWrapper::addBarPlot(const std::string& _label, std::vector<double>&& data,
                                   std::vector<std::string>&& format_labels) {
    map_vector_double[_label] = std::move(data);
    map_vector_string[_label] = std::move(format_labels);
    network_buffer_mtx[_label];
    window->addWidget(_label, std::make_unique<Widgets::BarPlot<double>>(
                                  _label, map_vector_double[_label], map_vector_string[_label],
                                  network_buffer_mtx[_label]));
}

void HttpWindowWrapper::addButton(const std::string& _label, const std::string& endpoint,
                                  drogon::HttpMethod method) {
    if (std::holds_alternative<HttpPoll::Poll>(connection)) {
        window->addWidget(_label, std::make_unique<Widgets::Button<>>(
                                      _label, endpoint, method,
                                      std::get<HttpPoll::Poll>(connection).getButtonCallback()));
    } else if (std::holds_alternative<Sse::SSE>(connection)) {
        window->addWidget(_label, std::make_unique<Widgets::Button<>>(
                                      _label, endpoint, method,
                                      std::get<Sse::SSE>(connection).getButtonCallback()));
    }
}

void HttpWindowWrapper::addImage(const std::string& _label, const std::string& endpoint) {
    buffer_container[_label].emplace<std::string>("");
    network_buffer_mtx[_label];
    window->addWidget(_label, std::make_unique<Widgets::Image<std::string>>(
                                  _label, std::get<std::string>(buffer_container[_label]),
                                  network_buffer_mtx[_label], endpoint));
    if (std::holds_alternative<HttpPoll::Poll>(connection)) {
        std::get<HttpPoll::Poll>(connection)
            .pollImage(endpoint, std::get<std::string>(buffer_container[_label]),
                       network_buffer_mtx[_label], window->widgets.at(_label)->is_data_available);
    } else if (std::holds_alternative<Sse::SSE>(connection)) {
        std::get<Sse::SSE>(connection)
            .pollImage(endpoint, std::get<std::string>(buffer_container[_label]),
                       network_buffer_mtx[_label], window->widgets.at(_label)->is_data_available);
    }
}
void HttpWindowWrapper::addTable(const std::string& _label, std::vector<std::string>&& header,
                                 std::vector<Widgets::Table::tableRowContainer>&& rows) {
    network_buffer_mtx[_label];
    vector_buffer_container[_label].emplace<std::vector<Widgets::Table::tableRowContainer>>(
        std::move(rows));
    window->addWidget(_label, std::make_unique<Widgets::Table>(
                                  _label, std::move(header),
                                  std::get<std::vector<Widgets::Table::tableRowContainer>>(
                                      vector_buffer_container[_label]),
                                  network_buffer_mtx[_label]));
}

uint32_t HttpWindowWrapper::window_count = 0;

HttpWindowWrapper::HttpWindowWrapper() : in_init_phase(true) {
    win_idx = window_count++;
    win_label = std::format("Window - {}", win_idx);
    window.emplace(win_label);
    const std::string_view default_host = "127.0.0.1";
    const std::string_view default_ep = "/events";
    std::copy(default_host.begin(), default_host.end(), this->host.begin());
    std::copy(default_ep.begin(), default_ep.end(), this->host_endpoint.begin());
    connection.emplace<std::monostate>();
    initFRs();
}
HttpWindowWrapper::HttpWindowWrapper(const std::string_view label, const std::string_view host,
                                     const std::string_view endpoint, const int port,
                                     const std::string_view connection_type)
    : in_init_phase(false) {
    win_idx = window_count++;
    win_label = label;
    window.emplace(win_label);
    const size_t h_len = std::min(host.size(), this->host.size() - 1);
    const size_t e_len = std::min(endpoint.size(), this->host_endpoint.size() - 1);
    std::copy_n(host.begin(), h_len, this->host.begin());
    std::copy_n(endpoint.begin(), e_len, this->host_endpoint.begin());
    this->port = port;
    this->use_sse = (connection_type == "sse");
    if (this->use_sse) {
        connection.emplace<Sse::SSE>(host, endpoint, port, "");
    } else {
        connection.emplace<HttpPoll::Poll>(host, endpoint, port);
    }
    initFRs();
}

HttpWindowWrapper::HttpWindowWrapper(const std::string_view label, const std::string_view host,
                                     const std::string_view endpoint, const int port,
                                     const std::string_view connection_type,
                                     const std::string_view auth_token)
    : in_init_phase(false), auth_token(auth_token) {
    win_idx = window_count++;
    win_label = label;
    window.emplace(win_label);
    const size_t h_len = std::min(host.size(), this->host.size() - 1);
    const size_t e_len = std::min(endpoint.size(), this->host_endpoint.size() - 1);
    std::copy_n(host.begin(), h_len, this->host.begin());
    std::copy_n(endpoint.begin(), e_len, this->host_endpoint.begin());
    this->port = port;
    this->use_sse = (connection_type == "sse");
    if (this->use_sse) {
        connection.emplace<Sse::SSE>(host, endpoint, port, auth_token);
    } else {
        connection.emplace<HttpPoll::Poll>(host, endpoint, port);
    }
    initFRs();
}
void HttpWindowWrapper::initFRs() {
    widget_updates_fr["text"] = [this](const Json::Value& params) {
        auto label_ = params["id"].asString();
        if (!window->isWidgetPresent(label_)) {
            addText(label_, params["data"].asString());
        } else [[likely]] {
            std::lock_guard<std::mutex> lock_(network_buffer_mtx[label_]);
            std::get<std::string>(buffer_container[label_]) = params["data"].asString();
            window->widgets.at(label_)->is_data_available.store(true);
        }
    };

    widget_updates_fr["radial_gauge"] = [this](const Json::Value& params) {
        auto label_ = params["id"].asString();
        if (!window->isWidgetPresent(label_)) {
            const float min_v = params.isMember("min") ? params["min"].asFloat() : 0.0f;
            const float max_v = params.isMember("max") ? params["max"].asFloat() : 100.0f;
            addRadialGauge(label_, params["data"].asFloat(), min_v, max_v);
        } else [[likely]] {
            std::get<std::atomic<float>>(buffer_container[label_]).store(params["data"].asFloat());
            window->widgets.at(label_)->is_data_available.store(true);
        }
    };
    widget_updates_fr["plot"] = [this](const Json::Value& params) {
        auto label_ = params["id"].asString();
        if (!window->isWidgetPresent(label_)) {
            addPlot(label_, params["data"].asFloat());
        } else [[likely]] {
            std::get<std::atomic<float>>(buffer_container[label_]).store(params["data"].asFloat());
            window->widgets.at(label_)->is_data_available.store(true);
        }
    };
    widget_updates_fr["bar_plot"] = [this](const Json::Value& params) {
        auto label_ = params["id"].asString();
        std::vector<double> data_vec;
        std::vector<std::string> data_label_vec;
        if (params.isMember("data") && params["data"].isArray()) {
            for (auto& elem : params["data"]) {
                data_vec.push_back(elem.asDouble());
            }
        }
        if (params.isMember("data_labels") && params["data_labels"].isArray()) {
            for (auto& elem : params["data_labels"]) {
                data_label_vec.push_back(elem.asString());
            }
        }
        std::lock_guard<std::mutex> lock(network_buffer_mtx[label_]);
        if (window->isWidgetPresent(label_)) {
            map_vector_double[label_] = std::move(data_vec);
            if (!data_label_vec.empty()) {
                map_vector_string[label_] = std::move(data_label_vec);
            }
        } else {
            addBarPlot(label_, std::move(data_vec), std::move(data_label_vec));
        }
        window->widgets.at(label_)->is_data_available.store(true);
    };
    widget_updates_fr["button"] = [this](const Json::Value& params) {
        auto label_ = params["id"].asString();
        if (params.isMember("endpoint")) {
            if (!window->isWidgetPresent(label_)) {
                const std::string endpoint = params["endpoint"].asString();
                std::string method_str =
                    params.isMember("method") ? params["method"].asString() : "GET";
                for (char& ch : method_str) {
                    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                }
                drogon::HttpMethod http_method = drogon::HttpMethod::Get;
                if (method_str == "POST") {
                    http_method = drogon::HttpMethod::Post;
                } else if (method_str == "PUT") {
                    http_method = drogon::HttpMethod::Put;
                } else if (method_str == "DELETE") {
                    http_method = drogon::HttpMethod::Delete;
                } else if (method_str == "PATCH") {
                    http_method = drogon::HttpMethod::Patch;
                } else if (method_str == "HEAD") {
                    http_method = drogon::HttpMethod::Head;
                } else if (method_str == "OPTIONS") {
                    http_method = drogon::HttpMethod::Options;
                }
                addButton(label_, endpoint, http_method);
            }
        }
    };
    widget_updates_fr["image"] = [this](const Json::Value& params) {
        auto label_ = params["id"].asString();
        if (params.isMember("endpoint")) {
            const std::string endpoint = params["endpoint"].asString();
            if (window->isWidgetPresent(label_)) {
                if (std::holds_alternative<HttpPoll::Poll>(connection)) {
                    std::get<HttpPoll::Poll>(connection)
                        .pollImage(endpoint, std::get<std::string>(buffer_container[label_]),
                                   network_buffer_mtx[label_],
                                   window->widgets.at(label_)->is_data_available);
                } else if (std::holds_alternative<Sse::SSE>(connection)) {
                    std::get<Sse::SSE>(connection)
                        .pollImage(endpoint, std::get<std::string>(buffer_container[label_]),
                                   network_buffer_mtx[label_],
                                   window->widgets.at(label_)->is_data_available);
                }
            } else {
                addImage(label_, endpoint);
            }
        }
    };
    widget_updates_fr["table"] = [this](const Json::Value& params) {
        auto label_ = params["id"].asString();
        std::vector<Widgets::Table::tableRowContainer> rows_vector;
        try {
            const auto& rows = params["data"];
            for (const auto& row : rows) {
                Widgets::Table::tableRowContainer temp_row;
                if (!row.isArray()) {
                    return;
                }
                for (const auto& col : row) {
                    if (col.isInt()) {
                        temp_row.push_back(Widgets::Table::tableRowContainerElem(col.asInt()));
                    } else if (col.isString()) {
                        temp_row.push_back(Widgets::Table::tableRowContainerElem(col.asString()));
                    } else if (col.isDouble()) {
                        temp_row.push_back(Widgets::Table::tableRowContainerElem(col.asFloat()));
                    } else {
                        std::println(
                            "Unhandled JSON type. isNull: {}, isObject: {}, isArray: {}, isBool: "
                            "{}",
                            col.isNull(), col.isObject(), col.isArray(), col.isBool());
                    }
                }
                rows_vector.emplace_back(std::move(temp_row));
            }

        } catch (const std::exception& e) {
            std::println("Error at parsing table json {}", e.what());
        }
        if (window->isWidgetPresent(label_)) {
            std::lock_guard<std::mutex> lock_(network_buffer_mtx[label_]);
            std::get<std::vector<Widgets::Table::tableRowContainer>>(
                vector_buffer_container[label_]) = std::move(rows_vector);
            window->widgets.at(label_)->is_data_available.store(true);
        } else {
            try {
                if (!params["header"].isArray()) {
                    return;
                }
                std::vector<std::string> headers;
                for (const auto& header : params["header"]) {
                    headers.emplace_back(header.asString());
                }
                addTable(label_, std::move(headers), std::move(rows_vector));
            } catch (const std::exception& e) {
                std::println("Error at parsing table json header{}", e.what());
            }
        }
    };

    widget_updates_fr["remove"] = [this](const Json::Value& params) {
        const Json::Value& targets =
            params.isMember("targets") ? params["targets"] : params["target"];
        if (!targets.isArray()) {
            return;
        }
        for (const auto& elem : targets) {
            auto key = elem.asString();
            window->removeWidget(key);
            network_buffer_mtx.erase(key);
            buffer_container.erase(key);
            vector_buffer_container.erase(key);
            map_vector_double.erase(key);
            map_vector_string.erase(key);
        }
    };

    widget_updates_fr["inline"] = [this](const Json::Value& params) {
        const std::string label_ = params["id"].asString();
        std::println("In Inline widget construction, parent - {}", label_);
        try {
            const auto& widgets = params["widgets"];
            bool is_first_widget = true;
            for (const auto& widget : widgets) {
                const std::string widget_id = widget["id"].asString();
                widget_updates_fr.at(widget["type"].asString())(widget);
                if (window->isWidgetPresent(widget_id)) {
                    window->widgets.at(widget_id)->isInline = !is_first_widget;
                }
                is_first_widget = false;
                std::println("Inline widget {} constructed with parent {}", widget_id, label_);
            }
        } catch (const std::exception& e) {
            std::println("Exception while constructing inline widgets");
        }
    };
}
void HttpWindowWrapper::parseJSON() {
    if (std::holds_alternative<HttpPoll::Poll>(connection)) {
        auto response = std::get<HttpPoll::Poll>(connection).getJSONBodyPtr();

        if (!response.first)
            return;

        std::lock_guard<std::mutex> _lock(*(response.second));
        const Json::Value& json = *(response.first);
        try {
            for (const Json::Value& widget_elem : json) {
                widget_updates_fr.at(widget_elem["type"].asString())(widget_elem);
            }
        } catch (const std::exception& e) {
            std::println("Exception while inferring widget type, in Http poll -> {}", e.what());
        }
    } else if (std::holds_alternative<Sse::SSE>(connection)) {
        while (std::get<Sse::SSE>(connection).is_data_available()) {
            auto json = std::get<Sse::SSE>(connection).getJson();
            if (json && json.value().isArray()) {
                for (const Json::Value& widget_elem : *json) {
                    try {
                        widget_updates_fr.at(widget_elem["type"].asString())(widget_elem);

                    } catch (const std::exception& e) {
                        std::println("Exception while inferring widget type, in SSE poll -> {}",
                                     e.what());
                    }
                }
            }
        }
    }
}

void HttpWindowWrapper::renderHeader() {
    ImGui::SetNextWindowSize(ImVec2(720.0f, 560.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin(win_label.c_str());

    if (in_init_phase) [[unlikely]] {
        if (Widgets::g_FontBold) {
            ImGui::PushFont(Widgets::g_FontBold);
        }
        ImGui::TextColored(ImVec4(0.19f, 0.78f, 0.86f, 1.0f), "NEW STREAM CONNECTION");
        if (Widgets::g_FontBold) {
            ImGui::PopFont();
        }
        ImGui::TextDisabled("Configure a server-driven UI endpoint (SSE or HTTP Poll)");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextUnformatted("Host Address");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputText("##HostURL", host.data(), host.size());

        ImGui::Spacing();
        ImGui::TextUnformatted("Endpoint Path");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputText("##HostEndpoint", host_endpoint.data(), host_endpoint.size());

        ImGui::Spacing();
        ImGui::TextUnformatted("Port");
        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputScalar("##Port", ImGuiDataType_U64, &port);

        ImGui::Spacing();
        ImGui::TextUnformatted("Transport Protocol");
        if (ImGui::RadioButton("Server-Sent Events (SSE)", use_sse)) {
            use_sse = true;
        }
        ImGui::SameLine(0.0f, 20.0f);
        if (ImGui::RadioButton("HTTP Polling", !use_sse)) {
            use_sse = false;
        }

        if (use_sse) {
            ImGui::Spacing();
            ImGui::TextUnformatted("Authorization Header (Optional)");
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::InputText("##AuthToken", auth_buf.data(), auth_buf.size());
        }

        ImGui::Spacing();
        ImGui::Spacing();
        if (ImGui::Button("Connect to Endpoint", ImVec2(-FLT_MIN, 40.0f))) {
            auth_token = auth_buf.data();
            if (use_sse) {
                connection.emplace<Sse::SSE>(host.data(), host_endpoint.data(),
                                             static_cast<int>(port), auth_token);
            } else {
                connection.emplace<HttpPoll::Poll>(host.data(), host_endpoint.data(),
                                                   static_cast<int>(port));
            }
            in_init_phase = false;
            ImGui::End();
            return;
        }
    }

    if (std::holds_alternative<std::monostate>(connection)) {
        ImGui::End();
        return;
    }

    // Connection status header bar inside the window
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const bool is_sse = std::holds_alternative<Sse::SSE>(connection);
        const ImU32 dot_col =
            is_sse ? IM_COL32(38, 198, 218, 255) : IM_COL32(102, 187, 106, 255);
        dl->AddCircleFilled(ImVec2(p.x + 6.0f, p.y + 9.0f), 4.5f, dot_col);
        ImGui::Dummy(ImVec2(14.0f, 18.0f));
        ImGui::SameLine(0.0f, 4.0f);
        ImGui::TextColored(
            is_sse ? ImVec4(0.19f, 0.78f, 0.86f, 1.0f) : ImVec4(0.40f, 0.73f, 0.42f, 1.0f),
            "%s", is_sse ? "SSE" : "POLL");
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::TextDisabled("%s:%llu%s", host.data(), static_cast<unsigned long long>(port),
                            host_endpoint.data());

        const std::string count_str =
            std::format("{} widgets", window ? window->widgets.size() : 0);
        const float count_w = ImGui::CalcTextSize(count_str.c_str()).x;
        const float avail = ImGui::GetWindowContentRegionMax().x;
        if (avail > count_w + 220.0f) {
            ImGui::SameLine(avail - count_w);
            ImGui::TextDisabled("%s", count_str.c_str());
        }
        ImGui::Separator();
        ImGui::Spacing();
    }

    if (std::holds_alternative<Sse::SSE>(connection)) {
        if (std::get<Sse::SSE>(connection).is_data_available()) {
            parseJSON();
        }
    } else if (std::holds_alternative<HttpPoll::Poll>(connection) &&
               std::get<HttpPoll::Poll>(connection).is_data_available()) {
        parseJSON();
        std::get<HttpPoll::Poll>(connection).parsed_data();  //  is_data_available -> false
    }

    if (window->widgets.empty()) {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const float offset_y = std::max(24.0f, avail.y * 0.38f);
        ImGui::Dummy(ImVec2(0.0f, offset_y));
        const char* msg = "Listening for incoming telemetry events...";
        const float tw = ImGui::CalcTextSize(msg).x;
        if (avail.x > tw) {
            ImGui::SetCursorPosX((avail.x - tw) * 0.5f + ImGui::GetCursorPosX());
        }
        ImGui::TextDisabled("%s", msg);
    } else {
        window->render();
    }

    ImGui::End();
}

}  // namespace ParseJson
