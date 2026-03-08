/*
This needs to someting like
    render a top menubar,
    check if there's a config file and connect to the servers in the config file

*/
#include <json/value.h>

#include <memory>
#include <optional>

#include "json.h"
namespace MainWindow {
extern std::vector<std::unique_ptr<ParseJson::HttpWindowWrapper>> poll_windows;
void init();
void renderWindows();
std::optional<Json::Value> loadConfig();
void clean();

}  // namespace MainWindow