#include "init.h"

#include <json/value.h>

#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <optional>
#include <sstream>

#include "imgui.h"
namespace MainWindow {

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
    if (auto config = checkForConfig(); config) {
        auto windows = config->getMemberNames();
        for (const auto& window : windows) {
        
        }
    }
}
std::optional<Json::Value> checkForConfig() {
    std::ifstream configfile("sentinel.json");
    std::stringstream buffer;
    if (configfile) {
        buffer << configfile.rdbuf();
        return Json::Value(buffer.str());
    }
    return std::nullopt;
    
}
void renderMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("reset")) {
            clean();
            init();
        }
    }
}
void clean() {}
}  // namespace MainWindow