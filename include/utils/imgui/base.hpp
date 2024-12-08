#ifndef FS_IMGUI_BASE
#define FS_IMGUI_BASE

#include <../external/imgui/imgui.h>
#include <../external/imgui/misc/cpp/imgui_stdlib.h>
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
        ImGui_ImplOpenGL3_Init();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
    }

    static void LoadFont(const std::string& tFontPath, const float tFontSize = 12.0f, const bool tClear = false) {
        ImGuiIO& io = ImGui::GetIO();
        if(tClear) io.Fonts->Clear();
        // Base font
        io.Fonts->AddFontFromFileTTF(tFontPath.c_str(), tFontSize, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    }

    static void NewFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    static void Render(const Window* tPtr) {
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

    float vec[3] = { 0,0,0 };
    static bool ColorEdit3(const char* tName, glm::vec3* tColor) {
        vec[0] = tColor->r;
        vec[1] = tColor->g;
        vec[2] = tColor->b;
        bool b = ImGui::ColorEdit3(tName, vec);
        if (b) {
            tColor->x = vec[0];
            tColor->y = vec[1];
            tColor->z = vec[2];
        }
        return b;
    }
    static bool DragFloat3(const char* tName, glm::vec3* tFloats, const float& tSpeed = 0.1f) {
        vec[0] = tFloats->r;
        vec[1] = tFloats->g;
        vec[2] = tFloats->b;
        bool b = ImGui::DragFloat3(tName, vec, tSpeed);
        if(b) {
            tFloats->r = vec[0];
            tFloats->g = vec[1];
            tFloats->b = vec[2];
        }
        return b;
    }

}

#endif // !FS_IMGUI_BASE