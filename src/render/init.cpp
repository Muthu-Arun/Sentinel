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
    try {
        std::string config_path = "sentinel.json";
        if (!std::filesystem::exists(config_path) &&
            std::filesystem::exists("../sentinel.json")) {
            config_path = "../sentinel.json";
        }
        simdjson::padded_string json = simdjson::padded_string::load(config_path);
        simdjson::ondemand::document doc = parser.iterate(json);
        for (auto window : doc.get_array()) {
            std::string_view authorization;  // auth is an optional elem
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
                window_id, host, endpoint, port, connection, authorization));
        }
    } catch (const std::exception& e) {
        std::println("Error Occured while parsing config file -> {}", e.what());
        poll_windows.emplace_back(std::make_unique<ParseJson::HttpWindowWrapper>());
    }
}

void renderWindows() {
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                 ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMainMenuBar()) {
        ImGui::TextColored(ImVec4(0.19f, 0.78f, 0.86f, 1.0f), "SENTINEL");
        ImGui::SameLine(0.0f, 16.0f);
        if (ImGui::BeginMenu("Connections")) {
            if (ImGui::MenuItem("New Connection Window")) {
                poll_windows.emplace_back(std::make_unique<ParseJson::HttpWindowWrapper>());
            }
            if (ImGui::MenuItem("Reload sentinel.json")) {
                clean();
                init();
            }
            ImGui::EndMenu();
        }
        ImGui::SameLine(0.0f, 12.0f);
        if (ImGui::SmallButton("+ Add Window")) {
            poll_windows.emplace_back(std::make_unique<ParseJson::HttpWindowWrapper>());
        }

        const ImGuiIO& io = ImGui::GetIO();
        const std::string status_str =
            std::format("Windows: {}  |  {:.0f} FPS ({:.2f} ms)", poll_windows.size(),
                        io.Framerate, 1000.0f / std::max(1.0f, io.Framerate));
        const float status_w = ImGui::CalcTextSize(status_str.c_str()).x;
        const float avail_w = ImGui::GetWindowWidth();
        if (avail_w > status_w + 260.0f) {
            ImGui::SameLine(avail_w - status_w - 16.0f);
            ImGui::TextDisabled("%s", status_str.c_str());
        }
        ImGui::EndMainMenuBar();
    }

    for (auto& window : poll_windows) {
        window->renderHeader();
    }
}
void clean() {
    poll_windows.clear();
}
}  // namespace MainWindow