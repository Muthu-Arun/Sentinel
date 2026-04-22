#include "init.h"

#include <json/value.h>

#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <memory>
#include <optional>
#include <print>
#include <sstream>

#include "imgui.h"
#include "json.h"
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
    if (auto config = loadConfig(); config && config.value() && config->isObject()) {
        auto windows = config.value().getMemberNames();
        for (const auto& window : windows) {
            std::println("Initialiazing window - {}", window);
            poll_windows.emplace_back(std::make_unique<ParseJson::HttpWindowWrapper>(
                window, config.value()[window]["host"].asString(),
                config.value()[window]["endpoint"].asString(),
                config.value()[window]["port"].asInt(),
                config.value()[window]["connection"].asString()));
        }
    } else {
        std::println("Failed to parse JSON");
        poll_windows.emplace_back(std::make_unique<ParseJson::HttpWindowWrapper>());
    }
}
std::optional<Json::Value> loadConfig() {
    std::ifstream configfile("sentinel.json");
    if (configfile) {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errs;

        // This is the step that actually "translates" the text into an Object
        bool ok = Json::parseFromStream(builder, configfile, &root, &errs);

        if (ok) {
            return root;
        } else {
            std::println("Parse Error: {}", errs);
            return std::nullopt;
        }
    }
    return std::nullopt;
}
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