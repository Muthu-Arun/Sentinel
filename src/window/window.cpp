#include "window.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>

#include "imgui.h"

namespace Window {
Window::Window(std::string_view label) : label(label) {}
void Window::render() {
    const float avail_width = ImGui::GetContentRegionAvail().x;
    const float spacing = ImGui::GetStyle().ItemSpacing.x + SpaceInBetweenWidgetX;
    const size_t n = widget_order.size();
    size_t i = 0;
    while (i < n) {
        size_t row_end = i + 1;
        while (row_end < n) {
            if (auto it = widgets.find(widget_order[row_end]);
                it != widgets.end() && it->second->isInline) {
                ++row_end;
            } else {
                break;
            }
        }
        const size_t row_count = row_end - i;
        const float item_width =
            (row_count > 1)
                ? std::max(140.0f, (avail_width - spacing * static_cast<float>(row_count - 1)) /
                                       static_cast<float>(row_count))
                : -1.0f;

        for (size_t k = i; k < row_end; ++k) {
            if (auto widget = widgets.find(widget_order[k]); widget != widgets.end()) {
                widget->second->inline_width = item_width;
                widget->second->draw();
            }
        }
        i = row_end;
    }
}

void Window::addWidget(const std::string_view id, std::unique_ptr<Widgets::Widget> widget) {
    if (widgets.emplace(id, std::move(widget)).second) {
        widget_order.emplace_back(id);
    }
}

void Window::updateWidget(const std::string& id, std::unique_ptr<Widgets::Widget> widget) {
    widgets[id] = std::move(widget);
}

bool Window::isWidgetPresent(const std::string& id) {
    return widgets.find(id) != widgets.end();
}
void Window::removeWidget(const std::string& id) {
    widgets.erase(id);
    std::erase(widget_order, id);
}
}  // namespace Window
