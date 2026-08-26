#pragma once
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "widget.h"
namespace Window {
class Window {
public:
    std::string label;  // Window Name
    std::unordered_map<std::string, std::unique_ptr<Widgets::Widget>> widgets;
    std::vector<std::string> widget_order;

    Window(std::string_view label);
    Window(const Window&) = delete;
    Window(Window&&) = delete;
    void render();
    void addWidget(std::string_view id, std::unique_ptr<Widgets::Widget> widget);
    void updateWidget(const std::string& id, std::unique_ptr<Widgets::Widget> widget);
    bool isWidgetPresent(const std::string& id);
    void removeWidget(const std::string& id);
};
}  // namespace Window
