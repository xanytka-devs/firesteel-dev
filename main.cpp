#include "engine/include/2d/text.hpp"
#include "engine/include/camera.hpp"
#include "engine/include/cubemap.hpp"
#include "engine/include/entity.hpp"
#include "engine/include/fbo.hpp"
#include "engine/include/input/input.hpp"
#include "engine/include/app.hpp"
#include "engine/include/utils/json.hpp"
#include "engine/include/audio/source.hpp"
#include "engine/include/audio/listener.hpp"
extern "C" {
# include "engine/external/lua/include/lua.h"
# include "engine/external/lua/include/lauxlib.h"
# include "engine/external/lua/include/lualib.h"
}
#include "engine/external/lua/LuaBridge/LuaBridge/LuaBridge.h"
using namespace Firesteel;
#include <stdio.h>
#include <time.h>
#include <../external/imgui/imgui-knobs.h>
#include <../external/imgui/ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

#include "include/light.hpp"
#include "include/editor_object.hpp"
#include "include/embeded.hpp"
#include "include/atmosphere.hpp"
#include "include/imgui/markdown.hpp"
#include "include/undo_buffer.hpp"
#include "include/config.hpp"
#include "include/audio_mixer.hpp"
#include "include/scene.hpp"

#pragma region Defines
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0, 0, -90));

int drawMode = 0;
int lightingMode = 3;
bool wireframeEnabled = false;
bool drawNativeUI = true;
bool drawImGUI = true;

void processInput(Window* tWin, float tDeltaTime, Joystick* joystick);
Text t;
static void displayLoadingMsg(std::string t_Msg, Shader* t_Shader, Window* t_Window);

//ImGui windows.
bool atmosphereOpen = false;
bool entityEditorOpen = true;
bool infoPanelOpen = false;
bool lightingOpen = false;
bool newsViewOpen = true;
bool sceneContentViewOpen = true;
bool sceneViewOpen = true;
bool soundMixerOpen = false;
bool undoHistoryOpen = false;

bool bgMuted = false;
bool sfxMuted = false;
float preMuteBgGain = 0.0f;
float preMuteSfxGain = 0.0f;

bool shdrParamMaterialGenPreview = false;
bool debugInfoOpen = true;
bool texViewOpen = false;
bool fullTexViewOpen = false;
bool alreadyShownImgWarn = false;
bool consoleOpen = false;
bool consoleDevMode = false;
bool switchedToDevConsole = false;
bool themeningOpen = false;
bool themeningEnabled = false;
bool inputTesterOpen = false;
bool joystickFastVibrTest = false;
bool joystickSlowVibrTest = false;
std::string consoleInput;
std::string consoleLog;
bool backupLogs = false;
bool didSaveCurPrj = true;
float mouseSenstivity = 0.1f;
bool mouseInvertY = false;

bool displayGizmos = true;
GLuint texID = 0;
bool drawSkybox = true;
int loadedW = 800, loadedH = 600;
glm::vec2 sceneWinSize;

float gamma = 1.f;
float shaderContrast = 1;
float shaderSaturation = 0;
float shaderHue = 0;
bool autoExposure = false;
float exposure = 1.0f, exposureRangeMin = 0.3f, exposureRangeMax = 2.5f, exposureMultiplyer = 1.1f;
bool bloom = true;
float bloomWeight = 1.f;
bool vigiette = false;
float vigietteSmoothness = 0.25f, vigietteStrength = 0.5f;
glm::vec3 vigietteColor = glm::vec3(0);

bool invertColors = false;
bool isAAEnabled = true;
bool useShaderKernel = false;
float shaderKernel[9] = {
    1.f/16,2.f/16,1.f/16,
    2.f/16,4.f/16,2.f/16,
    1.f/16,2.f/16,1.f/16
};
size_t shaderKernelSize = 9;
float temperature = 6500;
float temperatureMix = 1;

size_t heldEntityID = 0;
EditorObject* heldEntity;
EditorObject billboard;
ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;
ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;

const char* acts[4] = {
    u8"Перемещение объекта 'Тест'",
    u8"Обновлена модель объекта 'Тест'",
    u8"Объект 'Тест' переименов в 'Хрю'",
    u8"Обьект 'Хрю' был удалён"
};
const size_t randomLoggingPhrasesSIZE = 6;
const char* randomLoggingPhrases[randomLoggingPhrasesSIZE] = {
    "1.0 when?",
    "I could've given you a hint but I won't",
    "Yeah, I know that punction is absolutely random. But I don't really care",
    "Pineapple pizza",
    "Error: No error found",
    "Only the real ones remember v.0.1..."
};

bool needToScreenShot = false;
bool needToScreenShotEditor = false;
bool needToScreenShotEditorWaitFrame = true;
std::string screenShotPath = "";
int choosenScreenShotFormat = 0;
static void CreateScreenShot(unsigned int tWidth, unsigned int tHeight) {
    unsigned int width = tWidth;
    unsigned int height = tHeight;
    unsigned int bytesPerPixel = 4;
    std::vector<unsigned char> pixels(width * height * bytesPerPixel);
    if (pixels.empty()) {
        LOG_ERRR("Couldn't allocate buffer for screenshot");
        return;
    }
    if (StrEndsWith(screenShotPath.c_str(), ".ppm")) choosenScreenShotFormat = 0;
    else if (StrEndsWith(screenShotPath.c_str(), ".png")) choosenScreenShotFormat = 1;
    else if (StrEndsWith(screenShotPath.c_str(), ".jpg")) choosenScreenShotFormat = 2;
    else if (StrEndsWith(screenShotPath.c_str(), ".bmp")) choosenScreenShotFormat = 3;
    else if (StrEndsWith(screenShotPath.c_str(), ".tga")) choosenScreenShotFormat = 4;
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    width += 1;
    while (pixels.size() > static_cast<size_t>(width * height * 3)) pixels.pop_back();
    if (choosenScreenShotFormat == 0) {
        std::ofstream file(screenShotPath, std::ios::binary);
        if (file) {
            file << "P6\n" << width << " " << height << "\n255\n";
            for (int y = height - 1; y >= 0; --y) {
                file.write(reinterpret_cast<char*>(&pixels[y * static_cast<std::vector<uint8_t, std::allocator<uint8_t>>::size_type>(width) * 3]),
                    static_cast<std::streamsize>(width) * 3);
            }
            file.close();
            LOG_INFO("Created a screenshot at: " + screenShotPath);
        }
        else LOG_ERRR("Couldn't create a screenshot at: " + screenShotPath);
    } else {
        stbi_flip_vertically_on_write(true);
        bool couldCreate = true;

        if (choosenScreenShotFormat == 1) couldCreate = stbi_write_png(screenShotPath.c_str(), width, height, 3, pixels.data(), 3 * (width));
        else if (choosenScreenShotFormat == 2) couldCreate = stbi_write_jpg(screenShotPath.c_str(), width, height, 3, pixels.data(), 100);
        else if (choosenScreenShotFormat == 3) couldCreate = stbi_write_bmp(screenShotPath.c_str(), width, height, 3, pixels.data());
        else if (choosenScreenShotFormat == 4) couldCreate = stbi_write_tga(screenShotPath.c_str(), width, height, 3, pixels.data());

        if (!couldCreate) { LOG_ERRR("Couldn't create a screenshot at: " + screenShotPath); }
        else LOG_INFO("Created a screenshot at: " + screenShotPath);
        stbi_flip_vertically_on_write(false);
    }
}
static bool AskForScreenShot() {
    FileDialog fd;
    fd.filter = "All\0*.*\0PNG Image (*.png)\0*.png\0JPEG Image (*.jpg)\0*.jpg\0Portable Pixelmap (*.ppm)\0*.ppm\
\0Bitmap Picture (*.bmp)\0*.bmp\0TARGA Image (*.tga)\0*.tga\0";
    fd.filter_id = 2;
    screenShotPath = fd.save();
    return screenShotPath != "";
}
#pragma endregion

PointLight pLight{
    glm::vec3(-1.f, 0.75f, -1.f), // Position.
    // Lighting params.
    glm::vec3(0.2f), // Ambient.
    glm::vec3(0.5f), // Diffuse.
    glm::vec3(1.f), // Specular.
};
SpotLight sLight{
   glm::vec3(-2.f, 1.5f, -2.f), // Position.
   glm::vec3(0), // Direction.
   // Lighting params.
   glm::vec3(1.f), // Ambient.
   glm::vec3(5.0f), // Diffuse.
   glm::vec3(1.f), // Specular.
   // Attenuation.
   glm::cos(glm::radians(17.5f)), // Cutoff.
   glm::cos(glm::radians(20.5f)), // Outer cutoff.
};
bool sLightEnabled = false;
std::filesystem::path lastPath;

using namespace luabridge;
class EditorApp : public App {
    std::vector<Material> materialReg;
    std::vector<std::string> materialTypes;

    Texture billTex, pointLightBillTex;
    Framebuffer ppFBO, imguiFBO;
    Shader fboShader, defaultShader;
    imgui_conf imgConf;
    Joystick joystick;
    Cubemap sky;
    Scene scene;

    FSOAL::Source phoneAmbience;
    FSOAL::AudioLayer alSFX{};
    FSOAL::Source audio;
    FSOAL::AudioLayer alBG{};
    std::string newsTxtLoaded = "";

    ActionRegistry actions;
    lua_State* L = nullptr;
    bool luaInit = false;

    void setPosOfEnt(int tID, float tX, float tY, float tZ) {
        scene[tID].entity.transform.Position = glm::vec3(tX, tY, tZ);
    }
    void reloadShaders() {
        LOG_INFO("'Shader reload' sequence initialized...");
        LOG_INFO("Reloading base shaders");
        for (size_t i = 0; i < materialReg.size(); i++) {
            materialReg[i].reload();
            if(!materialReg[i].isLoaded()) materialReg[i].shader = defaultShader;
        }
        fboShader = Shader("res/fbo.vs", "res/fbo.fs");
        LOG_INFO("Setting parameters");
        LOG_INFO("'Shader reload' sequence has been completed");
    }

    void fboProcessing() {
        ppFBO.unbind();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        fboShader.enable();
        fboShader.setBool("bloom", bloom);
        fboShader.setBool("invertColors", invertColors);
        fboShader.setBool("vigiette", vigiette);
        fboShader.setBool("useKernel", useShaderKernel);
        if (useShaderKernel) for (size_t i = 0; i < shaderKernelSize; i++)
            fboShader.setFloat("shaderKernel[" + std::to_string(i) + "]", shaderKernel[i]);
        fboShader.setInt("AAMethod", isAAEnabled ? 1 : 0);
        fboShader.setFloat("exposure", exposure);
        fboShader.setFloat("gamma", gamma);
        fboShader.setFloat("contrast", shaderContrast);
        fboShader.setFloat("saturation", shaderSaturation);
        fboShader.setFloat("hue", shaderHue);
        fboShader.setFloat("bloomWeight", bloomWeight);
        fboShader.setFloat("temperature", temperature);
        fboShader.setFloat("temperatureMix", temperatureMix);
        fboShader.setFloat("vigietteStrength", vigietteStrength);
        fboShader.setFloat("vigietteSmoothness", vigietteSmoothness);
        fboShader.setVec3("vigietteColor", vigietteColor);
        fboShader.setVec2("screenSize", window.getSize());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ppFBO.getID(0));
        fboShader.setInt("screenTexture", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, ppFBO.getID(1));
        fboShader.setInt("bloomBlur", 2);
        if(drawImGUI) imguiFBO.bind();
        window.clearBuffers();
        if (autoExposure) {
            ppFBO.bindTexture();
            glGenerateMipmap(GL_TEXTURE_2D); // Generate mipmaps every frame
            glm::vec3 luminescence;
            glGetTexImage(GL_TEXTURE_2D, 10, GL_RGB, GL_FLOAT, &luminescence); // Read the value from the lowest mip level
            const float lum = 0.2126f * luminescence.r + 0.7152f * luminescence.g + 0.0722f * luminescence.b; // Calculate a weighted average

            const float adjSpeed = 0.05f;
            exposure = lerp(exposure, 0.5f / lum * exposureMultiplyer, adjSpeed); // Gradually adjust the exposure
            exposure = glm::clamp(exposure, exposureRangeMin, exposureRangeMax); // Don't let it go over or under a specified min/max range

            fboShader.setFloat("exposure", exposure);
        }
        ppFBO.drawQuad(&fboShader);
        /* Screenshot */ {
            if (needToScreenShot) {
                CreateScreenShot(static_cast<unsigned int>(ppFBO.getSize().x),
                    static_cast<unsigned int>(ppFBO.getSize().y));
                needToScreenShot = false;
            }
        }
        if (drawImGUI) {
            imguiFBO.unbind();
            window.clearBuffers();
        }
    }
    void drawNativeGUI() {
        t.draw(&materialReg[0].shader, std::to_string(fps), ppFBO.getSize(), glm::vec2(8.0f, 568.0f), glm::vec2(1.5f), glm::vec3(0));
        t.draw(&materialReg[0].shader, std::to_string(fps), ppFBO.getSize(), glm::vec2(10.0f, 570.0f), glm::vec2(1.5f), glm::vec3(1, 0, 0));

        t.draw(&materialReg[0].shader, currentDateTime(), ppFBO.getSize(), glm::vec2(8.0f, 553.0f), glm::vec2(1.f), glm::vec3(0));
        t.draw(&materialReg[0].shader, currentDateTime(), ppFBO.getSize(), glm::vec2(10.0f, 555.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));

        t.draw(&materialReg[0].shader, std::string("[M] VSync: ") + (window.getVSync() ? "ON" : "OFF"),
            ppFBO.getSize(), glm::vec2(8.0f, 538.0f), glm::vec2(1.f), glm::vec3(0));
        t.draw(&materialReg[0].shader, std::string("[M] VSync: ") + (window.getVSync() ? "ON" : "OFF"),
            ppFBO.getSize(), glm::vec2(10.0f, 540.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));

        if (glfwGetMouseButton(window.ptr(), 1) != GLFW_PRESS) {
            t.draw(&materialReg[0].shader, "[Ctrl+Wheel] FOV: " + std::to_string((int)camera.fov),
                ppFBO.getSize(), glm::vec2(8.0f, 523.0f), glm::vec2(1.f), glm::vec3(0));
            t.draw(&materialReg[0].shader, "[Ctrl+Wheel] FOV: " + std::to_string((int)camera.fov),
                ppFBO.getSize(), glm::vec2(10.0f, 525.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
        } else {
            t.draw(&materialReg[0].shader, "[Wheel] Relative movement",
                ppFBO.getSize(), glm::vec2(8.0f, 523.0f), glm::vec2(1.f), glm::vec3(0));
            t.draw(&materialReg[0].shader, "[Wheel] Relative movement",
                ppFBO.getSize(), glm::vec2(10.0f, 525.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
        }
    }
    void drawImGui(glm::mat4 view, glm::mat4 projection) {
        unsigned int imguiW = 0, imguiH = 0;
        // Start the Dear ImGui frame
        FSImGui::NewFrame();

        // Gui stuff.
        //Do fullscreen.
        ImGui::PopStyleVar(3);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGuiID dockspace_id = ImGui::GetID("Firesteel Editor");
        ImGui::Begin("Firesteel Editor", NULL, FSImGui::defaultDockspaceWindowFlags);
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), FSImGui::defaultDockspaceFlags);
        imguiW = static_cast<unsigned int>(ImGui::GetWindowViewport()->Size.x - 2);
        imguiH = static_cast<unsigned int>(ImGui::GetWindowViewport()->Size.y - 1);
        // Upper help menu.
        ImGui::PopStyleVar(1);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        if(ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu(u8"Файл")) {
                if (ImGui::MenuItem(u8"Сохранить")) {
                    FileDialog fd;
                    fd.filter = "All\0*.*\0Scene (*.scene.json)\0*.scene.json\0";
                    fd.filter_id = 2;
                    std::string res = fd.save();
                    if (res != "") {
                        scene.save(res.c_str(), &sky);
                        std::filesystem::current_path(lastPath);
                        LOG_INFO("Saved scene to \"" + res + "\".");
                        didSaveCurPrj = true;
                    }
                }
                if (ImGui::MenuItem(u8"Открыть")) {
                    FileDialog fd;
                    fd.filter = "All\0*.*\0Scene (*.scene.json)\0*.scene.json\0";
                    fd.filter_id = 2;
                    std::string res = fd.open();
                    if (res != "") {
                        LOG_INFO("Opening scene at: \"" + res + "\".");
                        std::filesystem::current_path(std::filesystem::current_path().parent_path());
                        scene.load(res.c_str(), &sky);
                        std::filesystem::current_path(lastPath);
                        LOG_INFO("Scene succesfully loaded.");
                        didSaveCurPrj = true;
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem(u8"Открыть демо-сцену")) {
                    scene = Scene("res\\demo.scene.json", &sky);
                    std::filesystem::current_path(lastPath);
                    LOG_INFO("Opened scene at \"res\\demo.scene.json\".");
                    didSaveCurPrj = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem(u8"Закрыть (Esc)")) window.close();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(u8"Окно")) {
                ImGui::Checkbox(u8"Атмосфера", &atmosphereOpen);
                ImGui::Checkbox(u8"История действий", &undoHistoryOpen);
                ImGui::Checkbox(u8"Консоль", &consoleOpen);
                ImGui::Checkbox(u8"Миксер аудио", &soundMixerOpen);
                ImGui::Checkbox(u8"Новости", &newsViewOpen);
                ImGui::Checkbox(u8"Просмотр сцены", &sceneViewOpen);
                ImGui::Checkbox(u8"Редактор объектов", &entityEditorOpen);
                ImGui::Checkbox(u8"Свет", &lightingOpen);
                ImGui::Checkbox(u8"Содержимое сцены", &sceneContentViewOpen);
                if(themeningEnabled) ImGui::Checkbox(u8"Редактор темы", &themeningOpen);
                else themeningOpen = false;
                if (ImGui::MenuItem(u8"Сбросить")) {
                    texViewOpen = false;
                    newsViewOpen = true;
                }
                ImGui::Separator();
                if(ImGui::MenuItem(u8"Скриншот")) { needToScreenShot = AskForScreenShot(); std::filesystem::current_path(lastPath); }
                if(ImGui::MenuItem(u8"Скриншот Редактора")) { needToScreenShotEditor = AskForScreenShot(); std::filesystem::current_path(lastPath); }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(u8"Помощь")) {
                if (ImGui::MenuItem(u8"О ПО")) infoPanelOpen = true;
                if (ImGui::MenuItem(u8"Сайт Xanytka Devs")) openURL("http://devs.xanytka.ru/");
                if (ImGui::MenuItem(u8"Вики FS Editor")) openURL("http://devs.xanytka.ru/wiki/fse");
                ImGui::Separator();
                if (ImGui::MenuItem(u8"Сообщить о проблеме")) openURL("https://forms.yandex.ru/u/6751fe9bd04688cc151223e8/");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(u8"Тестирование")) {
                if (ImGui::MenuItem(u8"Перезагрузить шейдеры")) reloadShaders();
                ImGui::Checkbox("Wireframe", &wireframeEnabled);
                ImGui::Separator();
                ImGui::Checkbox(u8"Генерация параметров шейдеров", &shdrParamMaterialGenPreview);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text(u8"Экспериментальная функция");
                    ImGui::Text(u8"Может резко снизить FPS при открытии параметров материалов в редакторе существ.");
                    ImGui::EndTooltip();
                }
                ImGui::Checkbox(u8"Консоль разработчика", &consoleDevMode);
                if (ImGui::Checkbox(u8"Поддержка тем", &themeningEnabled)) {
                    if (themeningEnabled) imgConf.setTheme();
                    else ImGui::StyleColorsDark();
                }
                ImGui::Checkbox(u8"Просмотр текстур", &texViewOpen);
                ImGui::Checkbox(u8"Создавать бэкапы логов", &backupLogs);
                ImGui::Checkbox(u8"Статистика потоков", &debugInfoOpen);
                ImGui::Checkbox(u8"Тест системы ввода", &inputTesterOpen);
                ImGui::Separator();
                if (ImGui::MenuItem(u8"Переключить Нативный UI")) { drawNativeUI = !drawNativeUI; }
                if (ImGui::MenuItem(u8"Выключить ImGui")) {
                    drawImGUI = false;
                    if (!alreadyShownImgWarn) LOG_INFO("ImGui rendering sequence disabled - To enable it back press LCtrl+LAlt+I");
                    alreadyShownImgWarn = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::Button("Guizmo")) displayGizmos = !displayGizmos;
            if (currentGizmoMode == ImGuizmo::LOCAL ? ImGui::Button("L") : ImGui::Button("W"))
                currentGizmoMode == ImGuizmo::LOCAL ? currentGizmoMode = ImGuizmo::WORLD : currentGizmoMode = ImGuizmo::LOCAL;
            ImGui::EndMenuBar();
        }
        ImGui::End();

        if(newsViewOpen) {
            ImGui::Begin("News", &newsViewOpen);
            FSImGui::MD::Text(newsTxtLoaded);
            ImGui::End();
        }
        if(infoPanelOpen) {
            ImGui::Begin("Info", &infoPanelOpen);
            auto windowWidth = ImGui::GetWindowSize().x;
            auto textWidth = ImGui::CalcTextSize(u8"Авторы редактора").x;

            textWidth = ImGui::CalcTextSize("------").x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::TextColored(ImVec4(1, 0.6f, 0, 1), "------");

            textWidth = ImGui::CalcTextSize("/         \\").x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::TextColored(ImVec4(1, 0.25f, 0, 1), "/         \\");

            textWidth = ImGui::CalcTextSize("|  FSE   |").x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::TextColored(ImVec4(1, 0.125f, 0.1f, 1), "|  FSE   |");

            textWidth = ImGui::CalcTextSize("\\         /").x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::TextColored(ImVec4(1, 0.25f, 0, 1), "\\         /");

            textWidth = ImGui::CalcTextSize("------").x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::TextColored(ImVec4(1, 0.6f, 0, 1), "------");

            textWidth = ImGui::CalcTextSize(u8"Авторы редактора").x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::Text(u8"Авторы редактора");
            ImGui::TextLinkOpenURL(u8"Саня Алабай", "https://github.com/sanyaalabai");
            textWidth = ImGui::CalcTextSize(u8"Авторы движка").x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::Text(u8"Авторы движка");
            ImGui::TextLinkOpenURL(u8"Саня Алабай", "https://github.com/sanyaalabai");
            textWidth = ImGui::CalcTextSize(u8"Сторонние библиотеки").x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::Text(u8"Сторонние библиотеки");
            ImGui::TextLinkOpenURL("Open Asset Import Library (assimp)", "https://github.com/assimp/assimp");
            ImGui::TextLinkOpenURL("FreeType", "https://freetype.org/");
            ImGui::TextLinkOpenURL("glad", "https://github.com/Dav1dde/glad");
            ImGui::TextLinkOpenURL("glfw", "https://github.com/glfw/glfw");
            ImGui::TextLinkOpenURL("imgui", "https://github.com/ocornut/imgui");
            ImGui::TextLinkOpenURL("ImGuizmo", "https://github.com/CedricGuillemet/ImGuizmo");
            ImGui::TextLinkOpenURL("imgui_markdown", "https://github.com/enkisoftware/imgui_markdown");
            ImGui::TextLinkOpenURL("ImGui Knobs", "https://github.com/altschuler/imgui-knobs");
            ImGui::TextLinkOpenURL("LuaBridge", "https://github.com/vinniefalco/LuaBridge");
            ImGui::TextLinkOpenURL("LibOVR", "https://developer.oculus.com/documentation/");
            ImGui::TextLinkOpenURL("OpenAL Soft", "https://github.com/kcat/openal-soft");
            ImGui::End();
        }
        if(sceneViewOpen) {
            ImGui::PopStyleVar(3);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);

            ImGui::Begin("Scene", &sceneViewOpen, ImGuiDockNodeFlags_NoTabBar);
            ImVec2 winSceneSize = ImGui::GetWindowSize();
            glm::vec2 winSize = glm::vec2(winSceneSize.x, winSceneSize.y);
            if (imguiFBO.getSize() != winSize) {
                ppFBO.scale(static_cast<int>(winSceneSize.x), static_cast<int>(winSceneSize.y));
                imguiFBO.scale(static_cast<int>(winSceneSize.x), static_cast<int>(winSceneSize.y));
                camera.aspect = ppFBO.aspect();
            }
            winSceneSize.y -= 20;
            ImGui::Image((ImTextureID)(size_t)imguiFBO.getID(), winSceneSize, ImVec2(0, 1), ImVec2(1, 0));

            ImGui::PopStyleVar(3);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));

            // Guizmos.
            if (displayGizmos) {
                if (heldEntity) {
                    glm::mat4 curModel = heldEntity->entity.getMatrix();
                    ImGuizmo::SetOwnerWindowName("Scene");
                    ImGuizmo::BeginFrame();

                    // imguizmo
                    ImGuizmo::SetOrthographic(false);
                    ImGuizmo::AllowAxisFlip(false);
                    ImGuizmo::SetDrawlist();
                    // ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);
                    ImGuizmo::SetRect(
                        ImGui::GetWindowPos().x,
                        ImGui::GetWindowPos().y,
                        imguiFBO.getSize().x,
                        imguiFBO.getSize().y
                    );

                    // Manipulate the model matrix
                    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
                        currentGizmoOperation, currentGizmoMode, glm::value_ptr(curModel));

                    float transl[3] = { 0,0,0 };
                    float rotat[3] = { 0,0,0 };
                    float scalel[3] = { 0,0,0 };
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(curModel), transl, rotat, scalel);
                    heldEntity->entity.transform.Position = float3ToVec3(transl);
                    heldEntity->entity.transform.Rotation = float3ToVec3(rotat);
                    heldEntity->entity.transform.Size = float3ToVec3(scalel);
                }
            }
            ImGui::End();
        }
        if(entityEditorOpen) {
            ImGui::Begin("Entity Editor", &entityEditorOpen);
            if (heldEntity) {
                ImGui::InputText("Name", &heldEntity->name);
                ImGui::SameLine();
                bool indeletion = false;
                if (ImGui::Button("X##delentity")) {
                    heldEntity->remove();
                    heldEntity = nullptr;
                    scene.entities.erase(std::next(scene.entities.begin(), heldEntityID));
                    heldEntityID = 0;
                    indeletion = true;
                }
                if (!indeletion) {
                    if (ImGui::CollapsingHeader(u8"Положение", ImGuiTreeNodeFlags_DefaultOpen)) {
                        FSImGui::DragFloat3(u8"Позиция", &heldEntity->entity.transform.Position);
                        FSImGui::DragFloat3(u8"Вращение", &heldEntity->entity.transform.Rotation);
                        FSImGui::DragFloat3(u8"Размер", &heldEntity->entity.transform.Size);
                    }
                    if (ImGui::CollapsingHeader(u8"Модель", ImGuiTreeNodeFlags_DefaultOpen)) {
                        if (heldEntity->entity.hasModel()) {
                            ImGui::Text(heldEntity->entity.model.path.c_str());
                            if (ImGui::Button("...##model_exchange")) {
                                LOG_INFO("Opening dialog for model selection (Entity:\"" + heldEntity->name + "\")");
                                FileDialog fd;
                                fd.filter = "All\0*.*\0OBJ Model (*.obj)\0*.obj\0FBX Model (*.fbx)\0*.fbx\0GLTF Model (*.gltf)\0*.gltf\0";
                                fd.filter_id = 2;
                                fd.path = heldEntity->entity.model.directory.c_str();
                                std::string modelPath = fd.open();
                                if (modelPath != "") {
                                    std::filesystem::current_path(lastPath);
                                    LOG_INFO("Model at \"" + modelPath + "\" does exist. Exchanging...");
                                    heldEntity->entity.clearMeshes();
                                    heldEntity->entity.loadFromFile(modelPath);
                                }
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("X##delmaterial")) heldEntity->entity.clearMeshes();
                            if (ImGui::CollapsingHeader(u8"Материал")) {
                                Material mat = heldEntity->material;
                                ImGui::Text((u8"Название: " + mat.name).c_str());
                                ImGui::Text((u8"Тип: " + materialTypes[mat.type]).c_str());
                                ImGui::Text((u8"Путь: " + mat.path).c_str());
                                ImGui::SameLine();
                                if (ImGui::Button("...##shdr_exchange")) {
                                    LOG_INFO("Opening dialog for shader selection (Entity:\"" + heldEntity->name + "\")");
                                    FileDialog fd;
                                    fd.filter = "All\0*.*\0Material (*.material.json)\0*.material.json\0";
                                    fd.filter_id = 2;
                                    fd.path = heldEntity->entity.model.directory.c_str();
                                    std::string shaderPath = fd.open();
                                    if (shaderPath != "") {
                                        std::filesystem::current_path(lastPath);
                                        LOG_INFO("Shader at \"" + shaderPath + "\" does exist. Exchanging...");
                                    }
                                }
                                if (shdrParamMaterialGenPreview) {
                                    GLint count, i, size;
                                    GLenum type;
                                    const GLsizei bufSize = 40;
                                    GLchar name[bufSize];
                                    GLsizei length;
                                    glGetProgramiv(mat.shader.ID, GL_ACTIVE_UNIFORMS, &count);
                                    ImGui::Text(("Active Uniforms: " + std::to_string(count)).c_str());

                                    mat.enable();
                                    for (i = 0; i < count; i++) {
                                        glGetActiveUniform(mat.shader.ID, (GLuint)i, bufSize, &length, &size, &type, name);
                                        GLint loc = glGetUniformLocation(mat.shader.ID, name);
                                        LOG_INFO(std::to_string(loc) + " " + name);
                                        GLint paramI;
                                        GLfloat paramF;
                                        float paramFs[4] = { 0,0,0,0 };
                                        bool val = false;
                                        if (StrSplit(name, '.').size() == 1) {
                                            switch (type) {
                                            case GL_FLOAT_MAT2:
                                                break;
                                            case GL_FLOAT_MAT3:
                                                break;
                                            case GL_FLOAT_MAT4:
                                                break;
                                            case GL_BOOL:
                                                glGetUniformiv(mat.shader.ID, loc, &paramI);
                                                val = paramI == 1;
                                                ImGui::Checkbox(name, &val);
                                                mat.setBool(name, val);
                                                break;
                                            case GL_INT:
                                                glGetUniformiv(mat.shader.ID, loc, &paramI);
                                                ImGui::DragInt(name, &paramI);
                                                mat.setInt(name, paramI);
                                                break;
                                            case GL_FLOAT:
                                                glGetUniformfv(mat.shader.ID, loc, &paramF);
                                                ImGui::DragFloat(name, &paramF);
                                                mat.setFloat(name, paramF);
                                                break;
                                            case GL_FLOAT_VEC2:
                                                glGetUniformfv(mat.shader.ID, loc, paramFs);
                                                FSImGui::DragLFloat2(name, paramFs[0], paramFs[1]);
                                                mat.setVec2(name, paramFs[0], paramFs[1]);
                                                break;
                                            case GL_FLOAT_VEC3:
                                                glGetUniformfv(mat.shader.ID, loc, paramFs);
                                                FSImGui::DragLFloat3(name, paramFs[0], paramFs[1], paramFs[2]);
                                                mat.setVec3(name, paramFs[0], paramFs[1], paramFs[2]);
                                                break;
                                            case GL_FLOAT_VEC4:
                                                glGetUniformfv(mat.shader.ID, loc, paramFs);
                                                FSImGui::DragLFloat4(name, paramFs[0], paramFs[1], paramFs[2], paramFs[3]);
                                                mat.setVec4(name, paramFs[0], paramFs[1], paramFs[2], paramFs[3]);
                                                break;
                                            default:
                                                ImGui::Text((std::string(" Name: ") + name + " Type: " + std::to_string(type)).c_str());
                                                break;
                                            }
                                        }
                                    }
                                }
                                ImGui::Text(u8"Текстуры");
                                for (size_t i = 0; i < heldEntity->entity.model.textures.size(); i++) {
                                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
                                    if (ImGui::ImageButton(("##" + heldEntity->entity.model.textures[i].type + std::to_string(i) + "_img").c_str(),
                                        (ImTextureID)(size_t)heldEntity->entity.model.textures[i].ID, ImVec2(20, 20))) {
                                        std::filesystem::current_path(lastPath);
                                        LOG_INFO("Opening dialog for texture selection (Entity:\"" + heldEntity->name + "\")");
                                        FileDialog fd;
                                        fd.filter = "All\0*.*\0PNG Image (*.png)\0*.png\0JPEG Image (*.jpg)\0*.jpg\0";
                                        fd.filter_id = 2;
                                        std::string texPath = fd.open();
                                        if (texPath != "") {
                                            LOG_INFO("Texture at \"" + texPath + "\" does exist. Exchanging...");
                                            heldEntity->entity.model.textures[i].remove();
                                            Texture t;
                                            t.ID = TextureFromFile(texPath);
                                            t.type = heldEntity->entity.model.textures[i].type;
                                            t.path = texPath;
                                            heldEntity->entity.model.textures[i] = t;
                                        }
                                    }
                                    ImGui::PopStyleVar(1);
                                    ImGui::SameLine();
                                    ImGui::Text((heldEntity->entity.model.textures[i].type + "_" + std::to_string(i)).c_str());
                                }
                            }
                        } else {
                            if (ImGui::Button("+ Add model")) {
                                LOG_INFO("Opening dialog for model selection (Entity:\"" + heldEntity->name + "\")");
                                FileDialog fd;
                                fd.filter = "All\0*.*\0OBJ Model (*.obj)\0*.obj\0FBX Model (*.fbx)\0*.fbx\0GLTF Model (*.gltf)\0*.gltf\0";
                                fd.filter_id = 2;
                                fd.path = heldEntity->entity.model.directory.c_str();
                                std::string modelPath = fd.open();
                                if (modelPath != "") {
                                    std::filesystem::current_path(lastPath);
                                    LOG_INFO("Model at \"" + modelPath + "\" does exist. Exchanging...");
                                    heldEntity->entity.loadFromFile(modelPath);
                                }
                            }
                        }
                    }
                }
            }
            ImGui::End();
        }
        if(sceneContentViewOpen) {
            ImGui::Begin("Scene Content", &sceneContentViewOpen);
            if(ImGui::CollapsingHeader("Scene")) {
                for (size_t i = 0; i < scene.entities.size(); i++) {
                    bool a = (heldEntityID == i);
                    if (ImGui::MenuItem((scene[static_cast<int>(i)].name + "##" + std::to_string(i)).c_str(), (const char*)0, &a)) {
                        heldEntityID = i;
                        heldEntity = &scene[static_cast<int>(i)];
                    }
                }
                if(ImGui::Button("+ Add entity")) {
                    EditorObject obj = EditorObject{ "New Entity", Entity(), materialReg[4] };
                    scene.entities.push_back(std::make_shared<EditorObject>(obj));
                }
            }
            ImGui::End();
        }
        if(atmosphereOpen) {
            ImGui::Begin("Atmosphere", &atmosphereOpen);
            if (ImGui::CollapsingHeader("Directional Light")) {
                FSImGui::DragFloat3("Direction", &scene.atmosphere.directionalLight.direction);
                ImGui::Separator();
                FSImGui::ColorEdit3("Ambient", &scene.atmosphere.directionalLight.ambient);
                FSImGui::ColorEdit3("Diffuse", &scene.atmosphere.directionalLight.diffuse);
                FSImGui::ColorEdit3("Specular", &scene.atmosphere.directionalLight.specular);
            }
            if (ImGui::CollapsingHeader("Fog")) {
                FSImGui::ColorEdit3("Color", &scene.atmosphere.fog.color);
                ImGui::Separator();
                ImGui::DragFloat("Start", &scene.atmosphere.fog.start, 0.1f);
                ImGui::DragFloat("End", &scene.atmosphere.fog.end, 0.1f);
                ImGui::DragFloat("Density", &scene.atmosphere.fog.density, 0.1f);
                ImGui::DragInt("Equation", &scene.atmosphere.fog.equation, 1, 0, 2);
            }
            if (ImGui::CollapsingHeader("Camera")) {
                FSImGui::DragFloat3("Position", &camera.pos);
                FSImGui::DragFloat3("Rotation", &camera.rot);
                float planes[2] = { camera.nearPlane, camera.farPlane };
                ImGui::DragFloat2("Planes", planes, 0.1f);
                ImGui::Checkbox("Is Perspective", &camera.isPerspective);
                ImGui::DragFloat("Field of View", &camera.fov, 0.01f);
            }
            if (ImGui::CollapsingHeader("Post-processing")) {
                ImGui::DragFloat("Gamma", &gamma, 0.05f);
                if (gamma < 0.1f) gamma = 0.1f;
                ImGui::Separator();

                ImGui::BeginDisabled(autoExposure);
                ImGui::DragFloat("Exposure", &exposure, 0.05f, 0, 10);
                ImGui::EndDisabled();
                ImGui::Checkbox("Auto exposure", &autoExposure);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text(u8"Экспериментальная функция");
                    ImGui::Text(u8"Может резко снизить FPS.");
                    ImGui::EndTooltip();
                }
                if (autoExposure) {
                    FSImGui::DragLFloat2("Exposure range", exposureRangeMin, exposureRangeMax);
                    ImGui::DragFloat("Exposure multiplyer", &exposureMultiplyer);
                }
                if (exposure < 0) exposure = 0;
                ImGui::Separator();

                ImGui::Checkbox("Bloom", &bloom);
                if (bloom) ImGui::DragFloat("Bloom weight", &bloomWeight, 0.1f);
                ImGui::Separator();

                ImGui::Text("Color correction");
                ImGui::DragFloat("Contrast", &shaderContrast, 0.01f, 0.01f, 2.0f);
                ImGui::DragFloat("Saturation", &shaderSaturation, 0.01f);
                ImGui::DragFloat("Hue", &shaderHue, 0.01f);
                ImGui::Checkbox("Invert colors", &invertColors);
                ImGui::DragFloat("Temperature", &temperature, 10, 1000, 40000);
                ImGui::DragFloat("Temperature mix", &temperatureMix, 0.01f, 0, 1);
                ImGui::Separator();

                ImGui::Checkbox("Vigiette", &vigiette);
                ImGui::DragFloat("Smoothness##vigiette", &vigietteSmoothness, 0.01f, 0, 1);
                ImGui::DragFloat("Strength##vigiette", &vigietteStrength, 0.01f, 0, 1);
                FSImGui::ColorEdit3("Color##vigiette", &vigietteColor);
                ImGui::Separator();

                ImGui::Text("Anti-Alising");
                ImGui::Checkbox("FXAA", &isAAEnabled);
                ImGui::Separator();

                ImGui::Text("Kernel");
                ImGui::Checkbox("Use shader kernel", &useShaderKernel);
                if (useShaderKernel) {
                    if (ImGui::Button("Reset")) {
                        shaderKernel[0] = shaderKernel[2] = shaderKernel[6] = shaderKernel[8] =
                            shaderKernel[1] = shaderKernel[3] = shaderKernel[5] = shaderKernel[7] = 0;
                        shaderKernel[4] = 1;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Blur")) {
                        shaderKernel[0] = shaderKernel[2] = shaderKernel[6] = shaderKernel[8] = 1.f / 16;
                        shaderKernel[1] = shaderKernel[3] = shaderKernel[5] = shaderKernel[7] = 2.f / 16;
                        shaderKernel[4] = 4.f / 16;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Sharpen")) {
                        shaderKernel[0] = shaderKernel[2] = shaderKernel[6] = shaderKernel[8] =
                            shaderKernel[1] = shaderKernel[3] = shaderKernel[5] = shaderKernel[7] = -1;
                        shaderKernel[4] = 9;
                    }
                    FSImGui::DragLFloat3("##shader_kernel_matrix_r1", shaderKernel[0], shaderKernel[1], shaderKernel[2]);
                    FSImGui::DragLFloat3("##shader_kernel_matrix_r2", shaderKernel[3], shaderKernel[4], shaderKernel[5]);
                    FSImGui::DragLFloat3("##shader_kernel_matrix_r3", shaderKernel[6], shaderKernel[7], shaderKernel[8]);
                }
            }
            ImGui::End();
        }
        if(soundMixerOpen) {
            ImGui::Begin("Sound Mixer", &soundMixerOpen);
            {
                ImGui::BeginGroup();
                ImGui::Text("Background");
                ImGui::BeginDisabled(bgMuted);
                if (ImGui::VSliderFloat("##BackgroundSlider", ImVec2(40, 200), &alBG.gain, 0.0f, 1.0f, "")) {
                    FSOAL::setGain(&audio, alBG);
                }
                std::string val = std::to_string(alBG.gain);
                val = val.substr(0, 5);
                ImGui::Text(val.c_str());
                if (ImGui::Knob("##BackgroundKnob", &alBG.pitch, 0.01f, 5.0f, 0.0f, "%.3f",
                    ImGuiKnobVariant_Wiper, 0.0f, ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput)) {
                    FSOAL::setPitch(&audio, alBG);
                }
                ImGui::EndDisabled();
                if (ImGui::Button("M##Background")) {
                    if (!bgMuted) preMuteBgGain = alBG.gain;
                    bgMuted = !bgMuted;
                    if (bgMuted) alBG.gain = 0;
                    else alBG.gain = preMuteBgGain;
                    FSOAL::setGain(&audio, alBG);
                }
                val = std::to_string(alBG.pitch);
                val = val.substr(0, 5);
                ImGui::Text(val.c_str());
                ImGui::EndGroup();
            }
            ImGui::SameLine();
            {
                ImGui::BeginGroup();
                ImGui::Text("SFX");
                ImGui::BeginDisabled(sfxMuted);
                if (ImGui::VSliderFloat("##SFXSlider", ImVec2(40, 200), &alSFX.gain, 0.0f, 1.0f, "")) {
                    FSOAL::setGain(&phoneAmbience, alSFX);
                }
                std::string val = std::to_string(alSFX.gain);
                val = val.substr(0, 5);
                ImGui::Text(val.c_str());
                if (ImGui::Knob("##SFXKnob", &alSFX.pitch, 0.01f, 5.0f, 0.0f, "%.3f",
                    ImGuiKnobVariant_Wiper, 0.0f, ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput)) {
                    FSOAL::setPitch(&phoneAmbience, alSFX);
                }
                ImGui::EndDisabled();
                if (ImGui::Button("M##SFX")) {
                    if (!sfxMuted) preMuteSfxGain = alSFX.gain;
                    sfxMuted = !sfxMuted;
                    if (sfxMuted) alSFX.gain = 0;
                    else alSFX.gain = preMuteSfxGain;
                    FSOAL::setGain(&phoneAmbience, alSFX);
                }
                val = std::to_string(alSFX.pitch);
                val = val.substr(0, 5);
                ImGui::Text(val.c_str());
                ImGui::EndGroup();
            }
            ImGui::End();
        }
        if(lightingOpen) {
            ImGui::Begin("Lighting", &lightingOpen);
            if (ImGui::CollapsingHeader("Point Light [1]")) {
                FSImGui::DragFloat3("Position##pl0", &pLight.position);
                ImGui::Separator();
                FSImGui::ColorEdit3("Ambient##pl0", &pLight.ambient);
                FSImGui::ColorEdit3("Diffuse##pl0", &pLight.diffuse);
                FSImGui::ColorEdit3("Specular##pl0", &pLight.specular);
            }
            if (ImGui::CollapsingHeader("Spot Light [1]")) {
                FSImGui::DragFloat3("Position##sl0", &sLight.position);
                FSImGui::DragFloat3("Direction##sl0", &sLight.direction);
                ImGui::Separator();
                FSImGui::ColorEdit3("Ambient##sl0", &sLight.ambient);
                FSImGui::ColorEdit3("Diffuse##sl0", &sLight.diffuse);
                FSImGui::ColorEdit3("Specular##sl0", &sLight.specular);
                ImGui::Separator();
                ImGui::DragFloat("Cut off##sl0", &sLight.cutOff);
                ImGui::DragFloat("Outer cut off##sl0", &sLight.outerCutOff);
            }
            ImGui::End();
        }
        if(undoHistoryOpen) {
            ImGui::Begin("Undo History", &undoHistoryOpen);
            for (size_t i = 0; i < 4; i++) {
                ImGui::BeginGroup();
                ImGui::Text(acts[i]);
                ImGui::EndGroup();
            }
            ImGui::End();
        }

        if(texViewOpen) {
            ImGui::Begin("Texture Viewer", &texViewOpen);
            float wW = ImGui::GetWindowWidth();
            int column = 3;
            if (wW > 400.f) column = 4;
            if (wW > 525.f) column = 5;
            if (wW > 650.f) column = 6;
            if (wW > 775.f) column = 7;
            if (wW > 900.f) column = 8;
            if (wW > 1025.f) column = 9;
            if (wW > 1150.f) column = 10;
            ImGui::BeginTable("Textures", column);
            glActiveTexture(GL_TEXTURE10);
            for (GLuint i = 0; i < 255; i++) {
                if (glIsTexture(i) == GL_FALSE) continue;
                glBindTexture(GL_TEXTURE_2D, i);
                ImGui::BeginGroup();
                ImGui::Image((ImTextureID)(size_t)i, ImVec2(100, 100));
                if (ImGui::Button(std::to_string(i).c_str())) {
                    texID = i;
                    fullTexViewOpen = true;
                }
                ImGui::EndGroup();
                ImGui::TableNextColumn();
            }

            ImGui::EndTable();
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
            ImGui::End();
        }
        if(fullTexViewOpen) {
            ImGui::Begin("Full Texture Viewer", &fullTexViewOpen);
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, texID);
            ImGui::Image((ImTextureID)(size_t)texID, ImVec2(ImGui::GetWindowWidth() * 0.9f, ImGui::GetWindowHeight() * 0.9f));
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
            ImGui::End();
        }
        if(consoleOpen) {
            ImGui::Begin("Console", &consoleOpen);
            if (consoleDevMode) {
                if (ImGui::Button(u8"Ошибки")) switchedToDevConsole = false;
                ImGui::SameLine();
                if (ImGui::Button(u8"Консоль")) switchedToDevConsole = true;
            } else switchedToDevConsole = false;
            if (consoleDevMode && switchedToDevConsole) {
                ImGui::Text(consoleLog.c_str());
                ImGuiInputTextFlags inputTextFlags =
                    ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCharFilter |
                    ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways;
                ImGui::PushItemWidth(-ImGui::GetStyle().ItemSpacing.x * 7);
                if (ImGui::InputText("##dev_input", &consoleInput, inputTextFlags, nullptr, this)) {
                    LOG_INFO("Executed cmd '" + consoleInput + "'");
                    consoleLog += "> " + consoleInput + "\n";
                    std::vector<std::string> consoleInputs = StrSplit(consoleInput, ' ');
                    if (consoleInputs[0] == "help") {
                        const char* hlpstr = "\
- help\n All available commands.\n\n\
- lua\n Execute following Lua code (output to CMD, not this console).\n\n\
- luaF\n Execute Lua code from following file (output to CMD, not this console).\n\n\
- exit\n Exits out of the app.\n\n\
- extencions (OpenAL,OpenGL,Vulkan)\n Available extencions for choosen platform.\n\n\
- amogus\n Don't you dare.\n";
                        consoleLog += hlpstr;
                        LOG(hlpstr);
                    } else if (consoleInputs[0] == "exit") {
                        window.close();
                    } else if (consoleInputs[0] == "amogus") {
                        const char* amstr = " Bruh, this was obvious.\n";
                        consoleLog += amstr;
                        openURL("https://www.youtube.com/watch?v=dQw4w9WgXcQ");
                        LOG(amstr);
                    } else if (consoleInputs[0] == "lua") {
                        std::string code;
                        for (size_t i = 1; i < consoleInputs.size(); i++) code += consoleInputs[i] + " ";
                        luaL_dostring(L, code.c_str());
                    } else if (consoleInputs[0] == "luaF") {
                        std::string path;
                        for (size_t i = 1; i < consoleInputs.size(); i++) path += consoleInputs[i] + " ";
                        luaL_dofile(L, path.c_str());
                    } else if (consoleInputs[0] == "extencions") {
                        if (consoleInputs[1] == "OpenAL") {
                            std::string exts = gimmeOALExtencions();
                            consoleLog += exts;
                            LOG(exts);
                        } else if (consoleInputs[1] == "OpenGL") {
                            std::string exts = gimmeOGLExtencions();
                            consoleLog += exts;
                            LOG(exts);
                        } else if (consoleInputs[1] == "Vulkan") {
                            std::string exts = gimmeVulkanExtencions();
                            consoleLog += exts;
                            LOG(exts);
                        } else {
                            const char* invExtStr = "Unknown extencion source";
                            consoleLog += invExtStr;
                            LOG(invExtStr);
                        }
                    } else {
                        const char* invstr = " Invalid command.\n";
                        consoleLog += invstr;
                        LOG(invstr);
                    }
                    consoleInput.clear();
                }
                ImGui::PopItemWidth();
            } else ImGui::Text(u8"Нет ошибок");
            ImGui::End();
        }
        if(themeningEnabled && themeningOpen) imgConf.drawThemeEditor(&themeningOpen);
        if(debugInfoOpen) {
            ImGui::Begin("Debug Info", &debugInfoOpen);
            ImGui::Text(("FPS: " + std::to_string(fps)).c_str());
            ImGui::Text(("Delta time: " + std::to_string(deltaTime)).c_str());
            ImGui::Separator();
            ImGui::Text("Display");
            ImGui::DragInt("Draw Mode", &drawMode, 1, 0, 9);
            ImGui::DragInt("Lighting", &lightingMode, 1, 0, 3);
            ImGui::Separator();
            ImGui::Text("Controls");
            ImGui::SliderFloat("Mouse Sensativity", &mouseSenstivity, 0.01f, 3.00f);
            ImGui::Checkbox("Invert mouse Y", &mouseInvertY);
            ImGui::End();
        }
        if(inputTesterOpen) {
            ImGui::Begin("Input Tester", &inputTesterOpen);
            ImGui::Text("Joystick ");
            if(joystick.isPresent()) {
                ImGui::SameLine();
                ImGui::Text(joystick.getName());
                if(joystick.getButtonCount()==18) {
                    ImGui::SameLine();
                    ImGui::Text(" (PlayStation)");
                } else if(joystick.getButtonCount()==14) {
                    ImGui::SameLine();
                    ImGui::Text(" (XBOX)");
                }
                ImGui::BeginDisabled();
                for (size_t i = 0; i < joystick.getAxesCount(); i++) {
                    float ax = joystick.getAxis(static_cast<int>(i));
                    ImGui::SliderFloat(("Axis #" + std::to_string(i)).c_str(), &ax, -1, 1);
                }
                for (size_t i = 0; i < joystick.getButtonCount(); i++) {
                    unsigned char b = joystick.getButton(static_cast<int>(i));
                    ImGui::Text(("Button #" + std::to_string(i) + ": " + std::to_string(b)).c_str());
                }
                ImGui::EndDisabled();
            } else {
                ImGui::BeginDisabled();
                ImGui::Text("No joystick found.");
                ImGui::EndDisabled();
                if(ImGui::Button("Update")) {
                    for (int i = 0; i < GLFW_JOYSTICK_LAST + 1; i++) {
                        joystick.initialize(i);
                        if(joystick.isPresent()) break;
                    }
                    if(joystick.isPresent()) joystick.printInfo();
                }
            }
            ImGui::End();
        }

        // Rendering
        FSImGui::Render(&window);
        /* Screenshot */ {
            if (needToScreenShotEditor) {
                if (needToScreenShotEditorWaitFrame) needToScreenShotEditorWaitFrame = false;
                else {
                    CreateScreenShot(imguiW, imguiH);
                    needToScreenShotEditor = false;
                }
            }
        }
    }

    std::string gimmeOGLExtencions() {
        std::string out = "OpenGL extencions:";
        GLint n = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &n);
        for (GLint i = 0; i < n; i++)
            out += std::string(" ") +
                (const char*)glGetStringi(GL_EXTENSIONS, i);
        return out;
    }
    std::string gimmeVulkanExtencions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        std::string out = "Vulkan extensions:";
        for (size_t i = 0; i < extensions.size(); i++)
            out += std::string(" ") + extensions[i];
        return out;
    }
    std::string gimmeOALExtencions() {
         return std::string("Extensions: ") + alcGetString(NULL, ALC_EXTENSIONS);
    }

    void reloadFonts(bool firstLoad = false) {
        if(firstLoad) imgConf.loadTheme(".theme.json");
        FSImGui::LoadFont(imgConf.baseFont, imgConf.baseFontSize, true);
        if(themeningEnabled && firstLoad) {
            LOG_INFO("ImGui themes enabled");
            imgConf.setTheme();
        }
        FSImGui::MD::LoadFonts("res/fonts/Ubuntu-Regular.ttf", "res/fonts/Ubuntu-Bold.ttf", 14, 15.4f);
        ImGui::GetIO().Fonts->Build();
        if (!firstLoad) {
            //Reconstruct device.
            ImGui_ImplOpenGL3_DestroyDeviceObjects();
            ImGui_ImplOpenGL3_CreateDeviceObjects();
        }
    }

    virtual void onInitialize() override {
        srand(static_cast<int>(glfwGetTime()));
        LOG_C(randomLoggingPhrases[rand() % (randomLoggingPhrasesSIZE)]);
        window.setIconFromMemory(emb::img_fse_logo, sizeof(emb::img_fse_logo)/ sizeof(*emb::img_fse_logo));
        window.setClearColor(glm::vec3(0.0055f, 0.002f, 0.f));
        // Getting ready text renderer.
        defaultShader = Shader(emb::shd_vrt_default, emb::shd_frg_default, false, "");
        materialReg.push_back(Material().load("res/mats/text.material.json"));
        TextRenderer::initialize();
        t.loadFont("res/fonts/vgasysr.ttf", 16);
        // OpenGL setup.
        displayLoadingMsg("Initializing OpenGL", &materialReg[0].shader, &window);
        // OpenAL setup.
        displayLoadingMsg("Initializing OpenAL", &materialReg[0].shader, &window);
#ifndef NDEBUG
        if(std::filesystem::exists("OpenAL32d.dll")) FSOAL::initialize();
#else
        if(std::filesystem::exists("OpenAL32.dll")) FSOAL::initialize();
#endif // !NDEBUG
        if(!FSOAL::globalInitState) { LOG_WARN("Couldn't initialize OpenAL (probably the library DLL is missing)."); }
        else {
            LOG_INFO("OpenAL context created:");
            LOG_INFO(std::string("	Device: ") + alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER));
        }
        audio.init("res/audio/elevator-music.mp3", 0.2f, true)->play();
        // Shaders.
        displayLoadingMsg("Compiling shaders", &materialReg[0].shader, &window);
        materialTypes.push_back("Lit");
        materialTypes.push_back("Unlit");
        materialReg.push_back(Material().load("res/mats/billboard.material.json"));
        materialReg.push_back(Material().load("res/mats/particle.material.json"));
        materialReg.push_back(Material().load("res/mats/skybox.material.json"));
        materialReg.push_back(Material().load("res/mats/lit_3d.material.json"));
        reloadShaders();
        displayLoadingMsg("Loading billboard system", &materialReg[0].shader, &window);
        billboard = EditorObject{ "Quad", Entity("res\\primitives\\quad.obj"), materialReg[1] };

        billTex = Texture{ TextureFromFile("res/yeah.png"), "texture_diffuse", "res/yeah.png" };
        pointLightBillTex = Texture{ TextureFromFile("res/billboards/point_light.png"), "texture_diffuse", "res/billboards/point_light.png" };
        // Load de scene.
        displayLoadingMsg("Loading scene", &materialReg[0].shader, &window);
        scene = Scene("res\\demo.scene.json", &sky);
        // Framebuffer.
        displayLoadingMsg("Creating FBO", &materialReg[0].shader, &window);
        ppFBO = Framebuffer(window.getSize(), 2);
        imguiFBO = Framebuffer(window.getSize());
        ppFBO.quad();
        if(!ppFBO.isComplete()) LOG_ERRR("Post-processing FBO isn't complete");
        if(!imguiFBO.isComplete()) {LOG_ERRR("ImGui Scene FBO isn't complete"); drawImGUI=false;}
        camera.farPlane = 1000;
        sceneWinSize = window.getSize();
        // ImGUI setup.
        LOG_INFO("Initializing ImGui");
        displayLoadingMsg("Initializing ImGui", &materialReg[0].shader, &window);
        FSImGui::Initialize(&window);
        ImGui::LoadIniSettingsFromDisk("editor.imgui.ini");
        std::filesystem::remove("imgui.ini");
        reloadFonts(true);
        LOG_INFO("ImGui ready");
        newsTxtLoaded = StrFromFile("News.md");
        // LuaBridge.
        displayLoadingMsg("Initializing Lua", &materialReg[0].shader, &window);
        if(std::filesystem::exists("lua54.lib")) {
            L = luaL_newstate();
            luaL_openlibs(L);
            lua_pcall(L, 0, 0, 0);
            {
                size_t cmd_v[] = {
                    CMD_F_BLACK   ,
                    CMD_F_BLUE    ,
                    CMD_F_GREEN   ,
                    CMD_F_CYAN    ,
                    CMD_F_RED     ,
                    CMD_F_PURPLE  ,
                    CMD_F_YELLOW  ,
                    CMD_F_GRAY    ,
                    CMD_F_LBLACK  ,
                    CMD_F_LBLUE   ,
                    CMD_F_LGREEN  ,
                    CMD_F_LCYAN   ,
                    CMD_F_LRED    ,
                    CMD_F_LPURPLE ,
                    CMD_F_LYELLOW ,
                    CMD_F_WHITE   ,

                    CMD_BG_BLACK  ,
                    CMD_BG_BLUE   ,
                    CMD_BG_GREEN  ,
                    CMD_BG_CYAN   ,
                    CMD_BG_RED    ,
                    CMD_BG_PURPLE ,
                    CMD_BG_YELLOW ,
                    CMD_BG_GRAY   ,
                    CMD_BG_LBLACK ,
                    CMD_BG_LBLUE  ,
                    CMD_BG_LGREEN ,
                    CMD_BG_LCYAN  ,
                    CMD_BG_LRED   ,
                    CMD_BG_LPURPLE,
                    CMD_BG_LYELLOW,
                    CMD_BG_WHITE
                };
                getGlobalNamespace(L)
                    .beginNamespace("fs")
                    .addConstant("version", GLOBAL_VER.c_str())
                    .beginNamespace("log")
                    .beginNamespace("f_color")
                    .addConstant("black", &cmd_v[0])
                    .addConstant("blue", &cmd_v[1])
                    .addConstant("green", &cmd_v[2])
                    .addConstant("cyan", &cmd_v[3])
                    .addConstant("red", &cmd_v[4])
                    .addConstant("purple", &cmd_v[5])
                    .addConstant("yellow", &cmd_v[6])
                    .addConstant("gray", &cmd_v[7])
                    .addConstant("lblack", &cmd_v[8])
                    .addConstant("lblue", &cmd_v[9])
                    .addConstant("lgreen", &cmd_v[10])
                    .addConstant("lcyan", &cmd_v[11])
                    .addConstant("lred", &cmd_v[12])
                    .addConstant("lpurple", &cmd_v[13])
                    .addConstant("lyellow", &cmd_v[14])
                    .addConstant("white", &cmd_v[15])
                    .endNamespace()
                    .beginNamespace("b_color")
                    .addConstant("black", &cmd_v[16])
                    .addConstant("blue", &cmd_v[17])
                    .addConstant("green", &cmd_v[18])
                    .addConstant("cyan", &cmd_v[19])
                    .addConstant("red", &cmd_v[20])
                    .addConstant("purple", &cmd_v[21])
                    .addConstant("yellow", &cmd_v[22])
                    .addConstant("gray", &cmd_v[23])
                    .addConstant("lblack", &cmd_v[23])
                    .addConstant("lblue", &cmd_v[24])
                    .addConstant("lgreen", &cmd_v[25])
                    .addConstant("lcyan", &cmd_v[26])
                    .addConstant("lred", &cmd_v[27])
                    .addConstant("lpurple", &cmd_v[28])
                    .addConstant("lyellow", &cmd_v[29])
                    .addConstant("white", &cmd_v[30])
                    .endNamespace()
                    .addFunction("info", Log::log_info)
                    .addFunction("warn", Log::log_warn)
                    .addFunction("error", Log::log_error)
                    .addFunction("crit", Log::log_critical)
                    .addFunction("custom", Log::log_c)
                    .endNamespace()
                    //.beginNamespace("scene")
                    //    .addFunction("setPostion", setPosOfEnt)
                    //.endNamespace()
                    .endNamespace();
            }
            if(std::filesystem::exists("res/scripts/oninit.lua")) luaL_dofile(L, "res/scripts/oninit.lua");
            if(std::filesystem::exists("res/scripts/components/rotator.lua")) luaL_dofile(L, "res/scripts/components/rotator.lua");
            getGlobal(L, "onstart")();
            luaInit = true;
        } else LOG_WARN("Couldn't initialize LuaBridge (probably the library DLL is missing).")
        // OpenAL.
        phoneAmbience.init("res/audio/tone.wav", 0.1f, true)->setPostion(10.f, 0.f, 10.f)->play();
        audio.init("res/audio/harbour-port-ambience.mp3", 0.2f, true)->play();
        lastPath=std::filesystem::current_path();
        window.setClearColor(glm::vec3(0));
        window.setTitle((didSaveCurPrj?"":"*") + lastPath.u8string() + " | Firesteel " + GLOBAL_VER);
        glfwRequestWindowAttention(window.ptr());
        joystick.update();
        joystick.printInfo();
        //glDisable(GL_FRAMEBUFFER_SRGB);
        window.setClearColor(glm::vec3(0));
    }
    virtual void onUpdate() override {
        if(!drawImGUI) if(ppFBO.getSize() != window.getSize()) {
                ppFBO.scale(window.getSize());
                camera.aspect = ppFBO.aspect();
            }
        camera.aspect = ppFBO.aspect();
        glViewport(0, 0, static_cast<GLsizei>(ppFBO.getWidth()), static_cast<GLsizei>(ppFBO.getHeight()));
        if(imgConf.shouldReloadFonts) {
            imgConf.shouldReloadFonts = false;
            reloadFonts();
        }
        /* Input processing */ {
            processInput(&window, deltaTime, &joystick);
            audio.setPostion(camera.pos);
            ppFBO.bind();
            window.clearBuffers();
            wireframeEnabled ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        glm::mat4 projection = camera.getProjection(1);
        glm::mat4 view = camera.getView();
        glm::mat4 model = glm::mat4(1.0f);
        /* Prerender processing */ {
            sLight.position = camera.pos;
            sLight.direction = camera.Forward;
            for (size_t i = 0; i < materialReg.size(); i++) {
                materialReg[i].enable();
                materialReg[i].setInt("drawMode", drawMode);
                if(materialReg[i].type == 0) {
                    //materialReg[i].setInt("lightingType", lightingMode);
                    materialReg[i].setInt("numPointLights", 1);
                    pLight.setParams(&materialReg[i].shader, 0);
                    materialReg[i].setInt("numSpotLights", sLightEnabled ? 1 : 0);
                    sLight.setParams(&materialReg[i].shader, 0);
                }
            }
            defaultShader.enable();
            defaultShader.setVec2("resolution", ppFBO.getSize());
        }
        scene.draw(&camera, &materialReg, &sky);
        /* Draw billboard */ {
            billboard.entity.transform.Position = glm::vec3(0.f, 5.f, 0.f);
            billTex.bind();
            materialReg[1].enable();
            materialReg[1].setVec3("color", glm::vec3(1));
            materialReg[1].setVec2("size", glm::vec2(1.f));
            billboard.draw();
            Texture::unbind();
        }
        /* Draw pseudo-gizmos */ {
            if (displayGizmos) {
                billboard.entity.transform.Position = pLight.position;
                glDepthFunc(GL_ALWAYS);
                pointLightBillTex.bind();
                materialReg[1].setVec3("color", pLight.diffuse);
                materialReg[1].setVec2("size", glm::vec2(0.25f));
                materialReg[1].setMat4("model", model);
                billboard.draw();
                Texture::unbind();
                glDepthFunc(GL_LESS);
            }
        }
        fboProcessing();
        if(drawNativeUI) drawNativeGUI();
        if(drawImGUI) drawImGui(view, projection);
        if(Keyboard::keyDown(KeyCode::F5)) reloadShaders();
    }
    virtual void onShutdown() override {
        if(luaInit) getGlobal(L, "onend")();
        // ImGui
        loadedW = window.getWidth();
        loadedH = window.getHeight();
        ImGui::SaveIniSettingsToDisk("editor.imgui.ini");
        FSImGui::Shutdown();
        if(themeningEnabled) imgConf.saveTheme(".theme.json");
        LOG_INFO("ImGui terminated");
        // OpenAL.
        phoneAmbience.remove();
        audio.remove();
        FSOAL::deinitialize();
        LOG_INFO("OpenAL terminated.");
        // Materials & objects
        scene.remove();
        for (size_t i = 0; i < materialReg.size(); i++)
            materialReg[static_cast<int>(i)].remove();
        materialReg.clear();
    }
};

static void saveConfig() {
    nlohmann::json txt;
    if(!newsViewOpen) txt["openedNews"] = GLOBAL_VER;
    else txt["openedNews"] = "false";

    txt["window"]["width"] = loadedW;
    txt["window"]["height"] = loadedH;

    txt["controls"]["mouseSensativity"] = mouseSenstivity;
    txt["controls"]["mouseInvertY"] = mouseInvertY;

    txt["dev"]["consoleDevMode"] = consoleDevMode;
    txt["dev"]["backupLogs"] = backupLogs;
    txt["dev"]["inputTesterOpen"] = inputTesterOpen;
    txt["dev"]["shdrParamMaterialGenPreview"] = shdrParamMaterialGenPreview;
    txt["dev"]["themeningEnabled"] = themeningEnabled;
    txt["dev"]["themeningOpen"] = themeningOpen;

    txt["layout"]["atmosphere"] = atmosphereOpen;
    txt["layout"]["console"] = consoleOpen;
    txt["layout"]["entityEditor"] = entityEditorOpen;
    txt["layout"]["infoPanel"] = infoPanelOpen;
    txt["layout"]["lighting"] = lightingOpen;
    txt["layout"]["metrics"] = debugInfoOpen;
    txt["layout"]["sceneContentView"] = sceneContentViewOpen;
    txt["layout"]["sceneView"] = sceneViewOpen;
    txt["layout"]["soundMixer"] = soundMixerOpen;
    txt["layout"]["undoHistory"] = undoHistoryOpen;

    std::ofstream o(".fsecfg");
    o << std::setw(4) << txt << std::endl;
}
static void loadConfig() {
    if(!std::filesystem::exists(".fsecfg")) return;
    std::ifstream ifs(".fsecfg");
    nlohmann::json txt = nlohmann::json::parse(ifs);
    if(!txt["openedNews"].is_null()) {
        if (txt["openedNews"] == GLOBAL_VER) newsViewOpen = false;
        else newsViewOpen = true;
    }
    if (!txt["window"].is_null()) {
        loadedW = txt["window"]["width"];
        loadedH = txt["window"]["height"];
    }
    if(!txt["controls"].is_null()) {
        mouseSenstivity = txt["controls"]["mouseSensativity"];
        mouseInvertY = txt["controls"]["mouseInvertY"];
    }
    if(!txt["dev"].is_null()) {
        if(!txt["dev"]["backupLogs"].is_null())                     backupLogs = txt["dev"]["backupLogs"];
        if(!txt["dev"]["consoleDevMode"].is_null())                 consoleDevMode = txt["dev"]["consoleDevMode"];
        if(!txt["dev"]["inputTesterOpen"].is_null())                inputTesterOpen = txt["dev"]["inputTesterOpen"];
        if(!txt["dev"]["shdrParamMaterialGenPreview"].is_null())    shdrParamMaterialGenPreview = txt["dev"]["shdrParamMaterialGenPreview"];
        if(!txt["dev"]["themeningEnabled"].is_null())               themeningEnabled = txt["dev"]["themeningEnabled"];
        if(!txt["dev"]["themeningOpen"].is_null())                  themeningOpen = txt["dev"]["themeningOpen"];
    }
    if(!txt["layout"].is_null()) {
        txt = txt.at("layout");
        if(!txt["atmosphere"].is_null())         atmosphereOpen = txt["atmosphere"];
        if(!txt["console"].is_null())            consoleOpen = txt["console"];
        if(!txt["entityEditor"].is_null())       entityEditorOpen = txt["entityEditor"];
        if(!txt["infoPanel"].is_null())          infoPanelOpen = txt["infoPanel"];
        if(!txt["lighting"].is_null())           lightingOpen = txt["lighting"];
        if(!txt["metrics"].is_null())            debugInfoOpen = txt["metrics"];
        if(!txt["sceneContentView"].is_null())   sceneContentViewOpen = txt["sceneContentView"];
        if(!txt["sceneView"].is_null())          sceneViewOpen = txt["sceneView"];
        if(!txt["soundMixer"].is_null())         soundMixerOpen = txt["soundMixer"];
        if(!txt["undoHistory"].is_null())        undoHistoryOpen = txt["undoHistory"];
    }
}

int main() {
    EditorApp app{};
    loadConfig();
    int r = app.start(("Firesteel " + GLOBAL_VER).c_str(), loadedW, loadedH, WS_NORMAL);
    FSOAL::deinitialize();
    LOG_INFO("Shutting down Firesteel App.");
    if(backupLogs) Log::destroyFileLogger();
    saveConfig();
    return r;
}

float speed_mult = 2.f;
bool pressedScrBtn = false;
bool pressedImgToggleBtn = false;
bool pressedNativeToggleBtn = false;
void processInput(Window* tWin, float tDeltaTime, Joystick* joystick) {
    if(Keyboard::keyDown(KeyCode::ESCAPE))
        tWin->close();

    if(Keyboard::keyDown(KeyCode::F1)) openURL("http://devs.xanytka.ru/wiki/fse");

    if((Keyboard::getKey(KeyCode::LEFT_CONTROL) || Keyboard::getKey(KeyCode::RIGHT_CONTROL))
        && (Keyboard::getKey(KeyCode::LEFT_ALT) || Keyboard::getKey(KeyCode::RIGHT_ALT))) {
        if(Keyboard::keyDown(KeyCode::I) && !pressedImgToggleBtn) {
            pressedImgToggleBtn = true;
            drawImGUI = !drawImGUI;
            if(!drawImGUI) {
                if(!alreadyShownImgWarn) LOG_INFO("ImGui rendering sequence disabled - To enable it back press LCtrl+LAlt+I");
                alreadyShownImgWarn = true;
            }
        }
        if(Keyboard::keyDown(KeyCode::U) && !pressedNativeToggleBtn) {
            pressedNativeToggleBtn = true;
            drawNativeUI = !drawNativeUI;
        }
    }
    if(Keyboard::keyUp(KeyCode::I)) pressedImgToggleBtn = false;
    if(Keyboard::keyUp(KeyCode::U)) pressedNativeToggleBtn = false;

    if(Keyboard::keyDown(KeyCode::F2) && !pressedScrBtn) {
        pressedScrBtn = true;
        std::filesystem::create_directory(lastPath.string() + "\\" + std::string("screenshots\\"));
        screenShotPath = lastPath.string() + "\\" + std::string("screenshots\\") + currentDateTime("%d-%m-%Y-") + StrReplace(currentDateTime("%X"), ':', '-') + ".jpg";
        needToScreenShot = true;
    }
    if(Keyboard::keyUp(KeyCode::F2)) pressedScrBtn = false;

    if(Keyboard::keyDown(KeyCode::E)) currentGizmoOperation = ImGuizmo::TRANSLATE;
    if(Keyboard::keyDown(KeyCode::R)) currentGizmoOperation = ImGuizmo::ROTATE;
    if(Keyboard::keyDown(KeyCode::T)) currentGizmoOperation = ImGuizmo::SCALE;

    if(Keyboard::keyDown(KeyCode::L)) currentGizmoMode = ImGuizmo::LOCAL;
    if(Keyboard::keyDown(KeyCode::K)) currentGizmoMode = ImGuizmo::WORLD;

    if((Keyboard::getKey(KeyCode::LEFT_CONTROL) || Keyboard::getKey(KeyCode::RIGHT_CONTROL)) && !Mouse::getButton(1)) {
        camera.fov -= Mouse::getWheelDY();
        if(camera.fov < 1.0f)
            camera.fov = 1.0f;
        if(camera.fov > 100.0f)
            camera.fov = 100.0f;
    }

    if(!Mouse::getButton(1)) tWin->setCursorMode(Window::CUR_NORMAL);
    else tWin->setCursorMode(Window::CUR_DISABLED);

    if(!Mouse::getButton(1) && !joystick->isPresent()) return;
    camera.pos += camera.Forward * (tDeltaTime * Mouse::getWheelDY() * 10.f);

    float dx = Mouse::getCursorDX(), dy = Mouse::getCursorDY();
    if(joystick->isPresent()) dx = joystick->getAxis(JoystickControls::AXIS_LEFT_STICK_X), dy = -joystick->getAxis(JoystickControls::AXIS_LEFT_STICK_Y);
    if(dx != 0 || dy != 0) {
        if(mouseInvertY) dy*=-1;
        camera.rot.y += dy*mouseSenstivity;
        camera.rot.z += dx*mouseSenstivity;
        if(camera.rot.y > 89.0f)
            camera.rot.y = 89.0f;
        if(camera.rot.y < -89.0f)
            camera.rot.y = -89.0f;
        if(camera.rot.z >= 360.0f)
            camera.rot.z -= 360.0f;
        if(camera.rot.z <= -360.0f)
            camera.rot.z += 360.0f;
        camera.update();
    }

    float velocity = 2.5f * tDeltaTime * speed_mult;
    bool isPressingMoveKey = false;

    if(Keyboard::keyDown(KeyCode::M))
        tWin->toggleVSync();

    if(Keyboard::getKey(KeyCode::W)) {
        camera.pos += camera.Forward * velocity;
        isPressingMoveKey = true;
    }
    if(Keyboard::getKey(KeyCode::S)) {
        camera.pos -= camera.Forward * velocity;
        isPressingMoveKey = true;
    }
    if(Keyboard::getKey(KeyCode::A)) {
        camera.pos -= camera.Right * velocity;
        isPressingMoveKey = true;
    }
    if(Keyboard::getKey(KeyCode::D)) {
        camera.pos += camera.Right * velocity;
        isPressingMoveKey = true;
    }

    if(Keyboard::getKey(KeyCode::LEFT_SHIFT) && isPressingMoveKey)
        speed_mult += 0.005f;
    else speed_mult = 1.f;

    if(Keyboard::getKey(KeyCode::SPACEBAR))
        camera.pos.y += velocity;
    if(Keyboard::getKey(KeyCode::LEFT_CONTROL))
        camera.pos.y -= velocity;

    FSOAL::Listener::setPosition(camera.pos);
    FSOAL::Listener::setRotation(camera.Forward, camera.Up);

    if(Keyboard::keyDown(KeyCode::V)) wireframeEnabled = !wireframeEnabled;
    if(Keyboard::keyDown(KeyCode::F)) sLightEnabled = !sLightEnabled;

    joystick->update();
}
static void displayLoadingMsg(std::string t_Msg, Shader* t_Shader, Window* t_Window) {
    t_Window->pollEvents();
    t_Window->clearBuffers();
    t.draw(t_Shader, "App is loading", t_Window->getSize(),
        glm::vec2(t_Window->getWidth() / 2.5f, t_Window->getHeight() / 2), glm::vec2(1.5f), glm::vec3(0.75f, 0.5f, 0.f));
    t.draw(t_Shader, t_Msg, t_Window->getSize(),
        glm::vec2(10, 15), glm::vec2(1.f), glm::vec3(1));
    t_Window->swapBuffers();
}