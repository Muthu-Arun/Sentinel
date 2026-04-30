#include <GLFW/glfw3.h>
#include <stdio.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <utility>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "json.h"
#include "poll.h"
#include "widget.h"
#include "init.h"

int main() {
    // 1. Setup GLFW
    HttpPoll::init();
    if (!glfwInit())
        return 1;

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

    // 2. Setup Dear ImGui context
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

    // 1. Load the Regular Font (Size 16.0f)

    // 2. Load the Bold Font (Size 16.0f)
    Widgets::g_FontBold = io.Fonts->AddFontFromFileTTF("fonts/Roboto-Bold.ttf", 20.0f, &config);

    Widgets::g_FontRegular = io.Fonts->AddFontFromFileTTF("fonts/Roboto-Regular.ttf", 16.0f, &config);
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // Build the font atlas (your backend usually does this automatically,
    // but if you are doing custom engine integration, call this)
    // io.Fonts->Build();
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    Widgets::init();
    // ParseJson::HttpWindowWrapper win;
    // win.initFRs();
    // Window::window win("hello");
    // win.addWidget("win1", std::make_unique<Widget::text<>>("sample"));
    // 3. Main loop
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
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
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

    return 0;
}