#include "window.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>

namespace Window {
Window::Window(std::string_view label) : label(label) {}
void Window::render() {
    for (const auto& id : widget_order) {
        if (auto widget = widgets.find(id); widget != widgets.end()) {
            widget->second->draw();
        }
    }
}

void Window::addWidget(const std::string_view id, std::unique_ptr<Widgets::Widget> widget) {
    const std::string widget_id(id);
    if (widgets.emplace(widget_id, std::move(widget)).second) {
        widget_order.emplace_back(widget_id);
    }
}

void Window::updateWidget(const std::string& id, std::unique_ptr<Widgets::Widget> widget) {
    widgets[id] = std::move(widget);
}

bool Window::isWidgetPresent(const std::string& id){
    return widgets.find(id) != widgets.end();
}
void Window::removeWidget(const std::string& id) {
    widgets.erase(id);
    std::erase(widget_order, id);
}
}  // namespace Window
