#include "../engine/include/app.hpp"
#include <../external/imgui/imgui.h>
#include <../external/imgui/misc/cpp/imgui_stdlib.h>
#include <../external/imgui/backends/imgui_impl_glfw.h>
#include <../external/imgui/backends/imgui_impl_opengl3.h>
#include "../external/imgui/imgui_internal.h"
#include <string>
#include "../engine/include/utils/utils.hpp"
#include "../engine/include/audio/source.hpp"
#include "../engine/include/audio/listener.hpp"
using namespace Firesteel;

class AudioExampleApp : public App {

    bool loadedAudio = false, justChangedOffset = false, isLooping=false;
    std::string curPath = "";
    std::filesystem::path lastPath;
    FSOAL::Source audio;
    float gain = 1, pitch = 1;

	virtual void onInitialize() override {
        lastPath = std::filesystem::current_path();
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF("res\\fonts\\Ubuntu-Regular.ttf", 13.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
        // Setup Dear ImGui style
        ImGui::StyleColorsDark(); // you can also use ImGui::StyleColorsClassic();
        // Choose backend
        ImGui_ImplGlfw_InitForOpenGL(window.ptr(), true);
        ImGui_ImplOpenGL3_Init();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
        ImGui::LoadIniSettingsFromDisk("audio_player.imgui.ini");
        std::filesystem::remove("imgui.ini");
        // OpenAL setup.
#ifndef NDEBUG
        if (std::filesystem::exists("OpenAL32d.dll")) FSOAL::initialize();
#else
        if (std::filesystem::exists("OpenAL32.dll")) FSOAL::initialize();
#endif // !NDEBUG
        if (!FSOAL::globalInitState) { LOG_WARN("Couldn't initialize OpenAL (probably the library DLL is missing)."); }
        else {
            LOG_INFO("OpenAL context created:");
            LOG_INFO(std::string("	Device: ") + alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER));
        }
	}
	virtual void onUpdate() override {
        window.clearBuffers();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Audio Player");
        if(audio.isInitialized()) {
            ImGui::BeginDisabled();
            ImGui::InputText("##audio_curpath", &curPath);
            ImGui::EndDisabled();
            ImGui::SameLine();
            if(ImGui::Button("...")) {
                FileDialog fd;
                fd.filter = "All\0*.*\0MP3 (*.mp3)\0*.mp3\0WAV (*.wac)\0*.wac\0FLAC (*.flac)\0*.flac\0";
                fd.filter_id = 1;
                std::string res = fd.open();
                if (res != "") {
                    LOG_INFO("Opening audio at: \"" + res + "\".");
                    std::filesystem::current_path(std::filesystem::current_path().parent_path());
                    gain = 1; pitch = 1;
                    curPath = res;
                    audio.init(curPath)->setGain(gain)->setLooping(isLooping)->setPitch(pitch)->play();
                    std::filesystem::current_path(lastPath);
                    LOG_INFO("Audio succesfully loaded.");
                }
            }
            if(audio.isPlaying()) {
                if(ImGui::Button(u8"Приостановить")) audio.pause();
            } else if(ImGui::Button(u8"Воспроизвести")) audio.play();
            ImGui::SameLine();
            if(ImGui::Button(u8"Стоп")) audio.stop();
            ImGui::SameLine();
            if(isLooping ? ImGui::Button(u8"Не повторять") : ImGui::Button(u8"Повторять")) isLooping = !isLooping;
            audio.setLooping(isLooping);
            float offset = audio.getOffset(), duration = audio.getDuration();
            int hourOff = (int)(offset/3600),   mntOff = (int)(offset/60-hourOff*60), secOff = (int)(offset-hourOff*3600-mntOff*60);
            int hourDur = (int)(duration/3600), mntDur = (int)(duration/60-hourDur*60),secDur = (int)(duration-hourDur*3600-mntDur*60);
            std::string finalResTxt = "";

            if((int)(hourOff/10)==0) finalResTxt += "0";
            finalResTxt += std::to_string(hourOff) + ":";
            if((int)(mntOff /10)==0) finalResTxt += "0";
            finalResTxt += std::to_string(mntOff) + ":";
            if((int)(secOff / 10) == 0) finalResTxt += "0";
            finalResTxt += std::to_string(secOff);
            finalResTxt += "/";

            if((int)(hourDur /10)==0) finalResTxt += "0";
            finalResTxt += std::to_string(hourDur) + ":";
            if((int)(mntDur /10)==0) finalResTxt += "0";
            finalResTxt += std::to_string(mntDur) + ":";
            if((int)(secDur / 10) == 0) finalResTxt += "0";
            finalResTxt += std::to_string(secDur);

            //ImGui::Text((std::to_string(offset) + "/" + std::to_string(duration)).c_str());
            ImGui::Text(finalResTxt.c_str());

            if(ImGui::SliderFloat("##offset", &offset, 0, duration, "")) {
                audio.pause();
                audio.setOffset(offset);
                justChangedOffset = true;
            } else if(justChangedOffset) {
                audio.play();
                justChangedOffset = false;
            }

            if(ImGui::SliderFloat(u8"Громкость", &gain, 0, 1)) audio.setGain(gain);
            if(ImGui::SliderFloat(u8"Высота", &pitch, 0, 3)) audio.setPitch(pitch);

            if((offset >= (duration-0.1)) && !audio.isLooping())
                audio.stop();
        } else {
            ImGui::Text(u8"Аудио не выбрано");
            ImGui::InputText("##audio_curpath1", &curPath);
            ImGui::SameLine();
            if (ImGui::Button("...")) {
                FileDialog fd;
                fd.filter = "All\0*.*\0MP3 (*.mp3)\0*.mp3\0WAV (*.wac)\0*.wac\0FLAC (*.flac)\0*.flac\0";
                fd.filter_id = 1;
                std::string res = fd.open();
                if (res != "") {
                    LOG_INFO("Opening audio at: \"" + res + "\".");
                    std::filesystem::current_path(std::filesystem::current_path().parent_path());
                    gain = 1; pitch = 1;
                    curPath = res;
                    audio.init(curPath)->setGain(gain)->setLooping(isLooping)->setPitch(pitch)->play();
                    std::filesystem::current_path(lastPath);
                    LOG_INFO("Audio succesfully loaded.");
                }
            }
            if(ImGui::Button(u8"Воспроизвести") && std::filesystem::exists(curPath)) {
                audio.init(curPath)->setGain(gain)->setLooping(isLooping)->setPitch(pitch)->play();
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"Стоп")) audio.stop();
            ImGui::SameLine();
            if (isLooping ? ImGui::Button(u8"Не повторять") : ImGui::Button(u8"Повторять")) isLooping = !isLooping;
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(window.ptr());
        }
        window.pollEvents();
	}
	virtual void onShutdown() override {
        FSOAL::deinitialize();
        ImGui::SaveIniSettingsToDisk("audio_player.imgui.ini");
	}
};

int main() {
    AudioExampleApp app{};
    int r = app.start("Audio Player", 400, 200, WS_NORMAL);
    FSOAL::deinitialize();
    LOG_INFO("Shutting down Firesteel App.");
    return r;
}