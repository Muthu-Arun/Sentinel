/*
This needs to someting like
    render a top menubar,
    check if there's a config file and connect to the servers in the config file

*/
#include <json/value.h>
#include <optional>
namespace MainWindow {
void init();
void renderMainMenu();
std::optional<Json::Value> checkForConfig();
void clean();

}  // namespace MainWindow