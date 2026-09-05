#include "init.h"

#include <json/value.h>

#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <memory>
#include <optional>
#include <print>
#include <sstream>
#include <string_view>

#include "imgui.h"
#include "json.h"
#include "simdjson.h"
namespace MainWindow {
std::vector<std::unique_ptr<ParseJson::HttpWindowWrapper>> poll_windows;
/*
JSON Config should look something like this
{
    window_name:{
    host: host domain or ip,
    endpoint: url endpoint,
    port: optional - default = 80,
    }
}
*/

void init() {
    simdjson::ondemand::parser parser;
    simdjson::padded_string json = simdjson::padded_string::load("sentinel.json");
    simdjson::ondemand::document doc = parser.iterate(json);
    try {
        for (auto window : doc.get_array()) {
            // auto window_obj = window->get_object();
            std::string_view authorization;
            std::string_view window_id = window["window"];
            std::string_view host = window["host"];
            std::string_view endpoint = window["endpoint"];
            std::string_view connection = window["connection"];
            auto err = window["authorization"].get(authorization);
            if (err) {
                std::println("Parsing auth failed, proceeding without auth");
            }
            uint32_t port = static_cast<uint32_t>(window["port"]);
            std::println("Parsed JSON values\n {}, {}, {}, {}, {}, {}", window_id, host, endpoint,
                         connection, authorization, port);

            poll_windows.emplace_back(std::make_unique<ParseJson::HttpWindowWrapper>(
                window_id, host, endpoint, port, connection));
        }
    } catch (const std::exception& e) {
        std::println("Error Occured while parsing config file -> {}", e.what());
        poll_windows.emplace_back(std::make_unique<ParseJson::HttpWindowWrapper>());
    }
}
/*
std::optional<simdjson::simdjson_result<simdjson::ondemand::document>> loadConfig() {
    std::ifstream configfile("sentinel.json");
    if (configfile) {
        std::stringstream buffer_content;
        buffer_content << configfile.rdbuf();
        std::string content = buffer_content.str();

        simdjson::ondemand::parser parser;
        auto doc = parser.iterate(content);
        return doc;
    }
    return std::nullopt;
}
*/
void renderWindows() {
    /*
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("reset")) {
            clean();
            init();
        }
    }
    */
    for (auto& window : poll_windows) {
        window->renderHeader();
    }
}
void clean() {
    poll_windows.clear();
}
}  // namespace MainWindow