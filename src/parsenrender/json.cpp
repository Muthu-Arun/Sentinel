#include "json.h"

#include <drogon/HttpTypes.h>
#include <json/value.h>

#include <algorithm>
#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
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

BarPlot:
    type = bar_plot
    data = array of doubles
    data_labels = array of strings
*/

void HttpWindowWrapper::addText(const std::string& _label, std::string_view data) {
    map_string[_label] = data;  // IG using [] oprator is not a problem here as if there aren't any
                                // we need to create one
    network_buffer_mtx[_label];
    window->addWidget(_label, std::make_unique<Widgets::Text<>>(_label, map_string[_label],
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
    map_float[_label] = data;
    window->addWidget(_label, std::make_unique<Widgets::RadialGauge<float>>(_label, min, max,
                                                                            map_float.at(_label)));
}
void HttpWindowWrapper::addPlot(const std::string& _label, float data,
                                Widgets::Plot<float>::type ptype) {
    map_float[_label] = data;
    window->addWidget(_label,
                      std::make_unique<Widgets::Plot<float>>(_label, ptype, map_float[_label]));
}
void HttpWindowWrapper::addBarPlot(const std::string& _label, const std::vector<double>& data,
                                   const std::vector<std::string>& format_labels) {
    map_vector_double[_label] = data;
    map_vector_string[_label] = format_labels;
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
    }
}

void HttpWindowWrapper::addImage(const std::string& _label, const std::string& endpoint) {
    map_string[_label];
    network_buffer_mtx[_label];
    window->addWidget(_label,
                      std::make_unique<Widgets::Image<std::string>>(
                          _label, map_string[_label], network_buffer_mtx[_label], endpoint));
    if (std::holds_alternative<HttpPoll::Poll>(connection)) {
        std::get<HttpPoll::Poll>(connection)
            .pollImage(endpoint, map_string[_label], network_buffer_mtx[_label],
                       window->widgets.at(_label)->is_data_available);
    }
}
/*
void parseDynamicJson(const Json::Value& root) {
    if (root.isArray()) {
    }
    if (root.isObject()) {
        // It's a map/dictionary: { "key": "value" }
        for (std::string& id : root.getMemberNames()) {
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
uint32_t HttpWindowWrapper::window_count = 0;

HttpWindowWrapper::HttpWindowWrapper() : in_init_phase(true) {
    win_idx = window_count++;
    win_label = std::format("Window - {}", win_idx);
    window.emplace(win_label);
    // host.fill('\0');
    // host_endpoint.fill('\0');
    connection.emplace<std::monostate>();
    initFRs();
}
HttpWindowWrapper::HttpWindowWrapper(const std::string& label, const std::string& host,
                                     const std::string& endpoint, const int port,
                                     const std::string& connection_type)
    : in_init_phase(false) {
    win_idx = window_count++;
    win_label = label;
    window.emplace(win_label);
    std::copy(host.begin(), host.end(), this->host.begin());
    std::copy(endpoint.begin(), endpoint.end(), this->host_endpoint.begin());
    this->port = port;
    // connection.emplace<HttpPoll::Poll>(this->host.data(), this->host_endpoint.data(),
    // this->port);
    if (connection_type == "sse") {
        connection.emplace<Sse::SSE>(host, endpoint, port);
    } else {
        connection.emplace<HttpPoll::Poll>(host, endpoint, port);
    }
    initFRs();
}
void HttpWindowWrapper::initFRs() {
    widget_updates_fr["text"] = [this](const std::string& label_, const Json::Value& params) {
        if (!window->isWidgetPresent(label_)) {
            addText(label_, params["data"].asString());
        } else [[likely]] {
            std::lock_guard<std::mutex> lock_(network_buffer_mtx[label_]);
            map_string[label_] = params["data"].asString();
            window->widgets.at(label_)->is_data_available.store(true);
        }
    };
    widget_updates_fr["radial_gauge"] = [this](const std::string& label_,
                                               const Json::Value& params) {
        if (!window->isWidgetPresent(label_)) {
            addRadialGauge(label_, params["data"].asFloat(), params["min"].asFloat(),
                           params["max"].asFloat());
        } else [[likely]] {
            map_float[label_] = params["data"].asFloat();
            window->widgets.at(label_)->is_data_available.store(true);
        }
    };
    widget_updates_fr["plot"] = [this](const std::string& label_, const Json::Value& params) {
        if (!window->isWidgetPresent(label_)) {
            addPlot(label_, params["data"].asFloat());
        } else [[likely]] {
            map_float[label_] = params["data"].asFloat();
            window->widgets.at(label_)->is_data_available.store(true);
        }
    };
    widget_updates_fr["bar_plot"] = [this](const std::string& label_, const Json::Value& params) {
        std::vector<double> data_vec;
        std::vector<std::string> data_label_vec;
        if (params.isMember("data") && params.isMember("data_labels")) {
            if (params["data"].isArray() && params["data_labels"].isArray()) {
                for (auto& elem : params["data"]) {
                    data_vec.push_back(elem.asDouble());
                }
                for (auto& elem : params["data_labels"]) {
                    data_label_vec.push_back(elem.asString());
                }
            } else {
                std::cerr << "Values Expected as Arrays for bar_plot\n";
            }
        }
        if (window->isWidgetPresent(label_)) {
            std::lock_guard<std::mutex> lock(network_buffer_mtx[label_]);
            map_vector_double[label_] = std::move(data_vec);
            map_vector_string[label_] = std::move(data_label_vec);
            window->widgets.at(label_)->is_data_available.store(true);
        } else {
            addBarPlot(label_, data_vec, data_label_vec);
        }
    };
    widget_updates_fr["button"] = [this](const std::string& label_, const Json::Value& params) {
        if (params.isMember("method") && params.isMember("endpoint")) {
            if (window->isWidgetPresent(label_)) {
                // Do nothing for now, a button does not need an updation
            } else {
                std::string endpoint = params["endpoint"].asString(),
                            method =
                                params["method"]
                                    .asString();  // For now assuming all methods are in Upper Case
                addButton(label_, endpoint,
                          drogon::HttpMethod::Get);  // Just to test for now NEED TO CHANGE
            }
        }
    };
    widget_updates_fr["image"] = [this](const std::string& label_, const Json::Value& params) {
        if (params.isMember("endpoint")) {
            if (window->isWidgetPresent(label_)) {
                std::string endpoint = params["endpoint"].asString();
                if (std::holds_alternative<HttpPoll::Poll>(connection)) {
                    std::get<HttpPoll::Poll>(connection)
                        .pollImage(endpoint, map_string[label_], network_buffer_mtx[label_],
                                   window->widgets.at(label_)->is_data_available);
                }
                // TODO Need to implement for SSE later
            } else {
                std::string endpoint = params["endpoint"].asString();
                addImage(label_, endpoint);
            }
        }
    };
    widget_updates_fr["remove"] = [this](const std::string& label_, const Json::Value& params) {
        if (!params.isArray()) [[unlikely]] {
            return;
        }
        for(auto& elem : params["target"]){
            auto key = elem.asString();
            window->removeWidget(key);
            network_buffer_mtx.erase(key);
            // Since keys are unique instead of checking, just erasing in all of them
            // As these buffers mostlikey be removed as the json parsing and updating will probably remain single threaded
            map_float.erase(key);
            map_int.erase(key);
            map_string.erase(key);
            map_vector_double.erase(key);
            map_vector_string.erase(key);
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
        for (const std::string& id : json.getMemberNames()) {
            widget_updates_fr.at(json[id]["type"].asString())(id, json[id]);
        }
    } else if (std::holds_alternative<Sse::SSE>(connection)) {
        while (std::get<Sse::SSE>(connection).is_data_available()) {
            auto json = std::get<Sse::SSE>(connection).getJson();
            if (json && json.value().isObject()) {
                for (const std::string& id : json.value().getMemberNames()) {
                    widget_updates_fr.at(json.value()[id]["type"].asString())(id, json.value()[id]);
                }
            }
        }
    }
}

void HttpWindowWrapper::renderHeader() {
    ImGui::Begin(win_label.c_str());
    if (in_init_phase) [[unlikely]] {
        ImGui::InputText("Host URL", host.data(), 250);
        ImGui::InputText("Host Endpoint", host_endpoint.data(), 250);
        ImGui::InputScalar("Port", ImGuiDataType_U64, &port);
        if (ImGui::Button("Enter Host URL")) {
            connection.emplace<HttpPoll::Poll>(host.data(), host_endpoint.data(), port);
            in_init_phase = false;
            ImGui::End();
            return;
        }
    }
    if (std::holds_alternative<std::monostate>(connection)) {
        ImGui::End();
        return;
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
    window->render();
    ImGui::End();
}

}  // namespace ParseJson