#include <curl/curl.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <utility>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "init.h"
#include "json.h"
#include "poll.h"
#include "widget.h"

int main() {
    // 1. Setup CURL (initializes WinSock on Windows) and GLFW
    curl_global_init(CURL_GLOBAL_DEFAULT);
    HttpPoll::init();
    if (!glfwInit()) {
        curl_global_cleanup();
        return 1;
    }

    // GL+GLSL versions
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Sentinel", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // Enable vsync

    // IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();  // (void)io;

    // Enable anti-aliasing (usually on by default, but good to ensure)
    io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines;

    // Setup font config for better quality (Oversampling)
    ImFontConfig config;
    config.OversampleH = 3;  // Renders the font at 3x horizontal resolution, then scales down
    config.OversampleV = 1;
    config.PixelSnapH = true;

    // Load the Regular Font (Size 16.0f) first so it becomes the default UI font,
    // followed by the Bold Font (Size 20.0f) for headings and gauges
    std::filesystem::path roboto_regular = "fonts/Roboto-Regular.ttf";
    std::filesystem::path roboto_bold = "fonts/Roboto-Bold.ttf";
    if (!std::filesystem::exists(roboto_regular) &&
        std::filesystem::exists("../fonts/Roboto-Regular.ttf")) {
        roboto_regular = "../fonts/Roboto-Regular.ttf";
        roboto_bold = "../fonts/Roboto-Bold.ttf";
    }
    if (std::filesystem::exists(roboto_regular) && std::filesystem::exists(roboto_bold)) {
        Widgets::g_FontRegular =
            io.Fonts->AddFontFromFileTTF(roboto_regular.string().c_str(), 16.0f, &config);
        Widgets::g_FontBold =
            io.Fonts->AddFontFromFileTTF(roboto_bold.string().c_str(), 20.0f, &config);
        io.FontDefault = Widgets::g_FontRegular;
    }

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    Widgets::init();
    // ParseJson::HttpWindowWrapper win;
    // win.initFRs();
    // Window::window win("hello");
    // win.addWidget("win1", std::make_unique<Widget::text<>>("sample"));
    MainWindow::init();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        MainWindow::renderWindows();
        // win.renderHeader();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.025f, 0.037f, 0.055f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
    MainWindow::clean();

    // Cleanup
    Widgets::cleanup();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    curl_global_cleanup();

    return 0;
}
