#include <../external/imgui/imgui.h>
#include <../external/imgui/backends/imgui_impl_glfw.h>
#include <../external/imgui/backends/imgui_impl_opengl3.h>
#include "../external/imgui/imgui_internal.h"
#include "../external/imgui/imgui_markdown.h"
#include <string>
#include "../utils.hpp"

namespace FSImGui {

    static ImGuiDockNodeFlags defaultDockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoWindowMenuButton;
    static ImGuiWindowFlags defaultDockspaceWindowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    static void Initialize(Window* tPtr) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_::ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard
            | ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_ViewportsEnable
            | ImGuiConfigFlags_::ImGuiConfigFlags_ViewportsEnable;
        // Setup Dear ImGui style
        ImGui::StyleColorsDark(); // you can also use ImGui::StyleColorsClassic();
        // Choose backend
        ImGui_ImplGlfw_InitForOpenGL(tPtr->ptr(), true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
    }

    static void LoadFont(std::string tFontPath, float tFontSize = 12.0f) {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        // Base font
        io.Fonts->AddFontFromFileTTF(tFontPath.c_str(), tFontSize, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    }

    static void NewFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    static void Render(Window* tPtr) {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(tPtr->ptr());
        }
    }

    static void Shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

}