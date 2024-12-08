#include "../include/2d/text.hpp"
#include "../include/camera.hpp"
#include "../include/cubemap.hpp"
#include "../include/entity.hpp"
#include "../include/fbo.hpp"
#include "../include/light.hpp"
#include "../include/atmosphere.hpp"
#include "../include/particles.hpp"
#include "../include/app.hpp"
using namespace Firesteel;

#include <stdio.h>
#include <time.h>
#include <../include/utils/imgui/markdown.hpp>
#include <../external/imgui/imgui-knobs.h>
#include <../external/imgui/ImGuizmo.h>
#include <../include/audio/source.hpp>
#include <../include/audio/listener.hpp>
#include <glm/gtc/type_ptr.hpp>
#define _GLIBCXX_USE_NANOSLEEP
#include <thread>
#include "../include/utils/json.hpp"
#include "../include/editor/undo_buffer.hpp"
#define __STDC_LIB_EXT1__
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/utils/stb_image_write.hpp"

extern "C" {
# include "../external/lua/include/lua.h"
# include "../external/lua/include/lauxlib.h"
# include "../external/lua/include/lualib.h"
}
#include <../external/lua/LuaBridge/LuaBridge/LuaBridge.h>

#pragma region Defines
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0, 0, -90));
float lastX = 800 / 2.0f;
float lastY = 600 / 2.0f;
bool firstMouse = true;

int drawSelectMod = 0; // 0=Textures, 1=Lightning
int drawMode = 0;
int lightingMode = 3;
bool wireframeEnabled = false;
bool drawNativeUI = true;
bool drawImGUI = true;

void processInput(Window* tWin, GLFWwindow* tPtr, float tDeltaTime);
void mouseCallback(GLFWwindow* window, double xposIn, double yposIn);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
Text t;
static void displayLoadingMsg(std::string t_Msg, Shader* t_Shader, Window* t_Window);
static std::string getDrawModName();
static std::string getLightingTypeName();

//ImGui windows.
bool texViewOpen = false;
bool fullTexViewOpen = false;
bool soundMixerOpen = false;
bool bgMuted = false;
bool sfxMuted = false;
float preMuteGain = 0.0f;
bool newsViewOpen = true;
bool sceneViewOpen = true;
bool threadsOpen = true;
bool lightingOpen = false;
bool atmosphereOpen = false;
bool entityEditorOpen = true;
bool sceneContentViewOpen = true;
bool undoHistoryOpen = false;
bool consoleOpen = false;
bool consoleDevMode = false;
bool switchedToDevConsole = false;
std::string consoleInput;
std::string consoleLog;
bool backupLogs = false;

bool didSaveCurPrj = true;

bool overrideAspect = false;
float clipSpace = 1.0f;

bool displayGizmos = true;
GLuint texID = 0;
bool drawSkybox = true;
int loadedW = 800, loadedH = 600;
glm::vec2 sceneWinSize = glm::vec2(loadedW,loadedH);
const std::string GLOBAL_VER = "0.2.0.4";

Entity* heldEntity;
ImGuizmo::MODE currentGizmoMode = ImGuizmo::LOCAL;
ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;

const char* acts[4] = {
    u8"Перемещение объекта 'Тест'",
    u8"Обновлена модель объекта 'Тест'",
    u8"Объект 'Тест' переименов в 'Хрю'",
    u8"Обьект 'Хрю' был удалён"
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
    }
    else {
        stbi_flip_vertically_on_write(true);
        if (choosenScreenShotFormat == 1) {
            if (!stbi_write_png(screenShotPath.c_str(), width, height, 3, pixels.data(), 3 * (width))) {
                LOG_ERRR("Couldn't create a screenshot at: " + screenShotPath);
            } else LOG_INFO("Created a screenshot at: " + screenShotPath);
        }
        else if (choosenScreenShotFormat == 2) {
            if (!stbi_write_jpg(screenShotPath.c_str(), width, height, 3, pixels.data(), 100)) {
                LOG_ERRR("Couldn't create a screenshot at: " + screenShotPath);
            } else LOG_INFO("Created a screenshot at: " + screenShotPath);
        }
        else if (choosenScreenShotFormat == 3) {
            if (!stbi_write_bmp(screenShotPath.c_str(), width, height, 3, pixels.data())) {
                LOG_ERRR("Couldn't create a screenshot at: " + screenShotPath);
            } else LOG_INFO("Created a screenshot at: " + screenShotPath);
        }
        else if (choosenScreenShotFormat == 4) {
            if (!stbi_write_tga(screenShotPath.c_str(), width, height, 3, pixels.data())) {
                LOG_ERRR("Couldn't create a screenshot at: " + screenShotPath);
            } else LOG_INFO("Created a screenshot at: " + screenShotPath);
        }
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

#pragma region Timers
bool threadRuntime[3] = { false,false, false };
int _gpuThreadTime = 0;
int _gpuThreadTimeNow = 0;
int _gpuThreadTimeBefore = 0;
static void _gpuTTCall() {
    _gpuThreadTimeNow+=1;
}
static void _gpuThreadTimer() {
    threadRuntime[0] = true;
    _gpuThreadTimeNow = 0;
    while (threadRuntime[0]) {
        _gpuTTCall();
    }
    _gpuThreadTime = (_gpuThreadTimeNow - _gpuThreadTimeBefore) / 1000;
    _gpuThreadTimeBefore = _gpuThreadTimeNow;
}
int _drawThreadTime = 0;
int _drawThreadTimeNow = 0;
int _drawThreadTimeBefore = 0;
static void _drawTTCall() {
    _drawThreadTimeNow += 1;
}
static void _drawThreadTimer() {
    threadRuntime[1] = true;
    _drawThreadTimeNow = 0;
    while (threadRuntime[1]) {
        _drawTTCall();
    }
    _drawThreadTime = (_drawThreadTimeNow - _drawThreadTimeBefore) / 1000;
    _drawThreadTimeBefore = _drawThreadTimeNow;
}
int _imguiThreadTime = 0;
int _imguiThreadTimeNow = 0;
int _imguiThreadTimeBefore = 0;
static void _imguiTTCall() {
    _imguiThreadTimeNow += 1;
}
static void _imguiThreadTimer() {
    threadRuntime[2] = true;
    _imguiThreadTimeNow = 0;
    while (threadRuntime[2]) {
        _imguiTTCall();
    }
    _imguiThreadTime = (_imguiThreadTimeNow - _imguiThreadTimeBefore) / 1000;
    _imguiThreadTimeBefore = _imguiThreadTimeNow;
}
#pragma endregion

Atmosphere atmos {
    Atmosphere::DirectionalLight{
        glm::vec3(-0.2f, -1.0f, -0.3f), // Direction.
        // Lighting params.
        glm::vec3(0.f), // Ambient.
        glm::vec3(0.f), // Diffuse.
        glm::vec3(10.f) // Specular.
    }
};
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
static void updateLightInShader(Shader* tShader) {
    tShader->enable();
    atmos.setParams(tShader);
    tShader->setInt("numPointLights", 1);
    pLight.setParams(tShader, 0);
    tShader->setInt("numSpotLights", sLightEnabled ? 1 : 0);
    sLight.setParams(tShader, 0);
}

class EditorApp : public App {
    Texture billTex, pointLightBillTex;
    Framebuffer ppFBO, imguiFBO;
    Shader textShader, modelShader, fboShader, billboardShader, particlesShader, skyShader;
    ParticleSystem pS = ParticleSystem(glm::vec3(0, 0, 0), 500, "res/particle.png", false);
    Entity backpack, quad, box, phoneBooth;
    Cubemap sky;
    FSOAL::Source phoneAmbience;
    FSOAL::AudioLayer alSFX{};
    FSOAL::Source audio;
    FSOAL::AudioLayer alBG{};
    std::string newsTxtLoaded = "";
    ActionRegistry actions;
    lua_State* L = nullptr;
    virtual void onInitialize() override {
        window.setIcon("app.png");
        window.setClearColor(glm::vec3(0.0055f, 0.002f, 0.f));
        // Getting ready text renderer.
        textShader = Shader("res/text.vs", "res/text.fs");
        TextRenderer::initialize();
        t.loadFont("res/fonts/vgasysr.ttf", 16);
        // OpenGL setup.
        displayLoadingMsg("Initializing OpenGL", &textShader, &window);
        glfwSetCursorPosCallback(window.ptr(), mouseCallback);
        glfwSetScrollCallback(window.ptr(), scrollCallback);
        // OpenAL setup.
        displayLoadingMsg("Initializing OpenAL", &textShader, &window);
        FSOAL::initialize();
        audio.init("res/audio/elevator-music.mp3", 0.2f, true)->play();
        // OpenAL info.
        LOG_INFO("OpenAL context created:");
        LOG_INFO(std::string("	Device: ") + alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER) + "\n");
        // Shaders.
        displayLoadingMsg("Compiling shaders", &textShader, &window);
        modelShader = Shader("res/model.vs", "res/model.fs");
        fboShader = Shader("res/fbo.vs", "res/fbo.fs");
        billboardShader = Shader("res/billboard.vs", "res/billboard.fs");
        particlesShader = Shader("res/particles.vs", "res/particles.fs");
        skyShader = Shader("res/skybox.vs", "res/skybox.fs");
        modelShader.enable();
        modelShader.setFloat("material.shininess", 64);
        modelShader.setFloat("material.skyboxRefraction", 1.00f / 1.52f);
        modelShader.setFloat("material.skyboxRefractionStrength", 0.025f);
        pS.init();

        // Load models.
        //displayLoadingMsg("Loading city", &textShader, &window);
        //Transform city("res/city/scene.gltf");
        displayLoadingMsg("Loading backpack", &textShader, &window);
        backpack = Entity("res\\backpack\\backpack.obj", glm::vec3(0.f, 0.f, -1.f), glm::vec3(0), glm::vec3(0.5));
        displayLoadingMsg("Loading quad", &textShader, &window);
        quad = Entity("res\\primitives\\quad.obj");
        displayLoadingMsg("Loading box", &textShader, &window);
        box = Entity("res\\box\\box.obj", glm::vec3(-2.f, 0.f, 1.f));
        displayLoadingMsg("Loading phone booth", &textShader, &window);
        phoneBooth = Entity("res\\phone-booth\\X.obj", glm::vec3(10.f, 0.f, 10.f), glm::vec3(0), glm::vec3(0.5f));
        billTex = Texture{ TextureFromFile("res/yeah.png"), "texture_diffuse", "res/yeah.png" };
        pointLightBillTex = Texture{ TextureFromFile("res/billboards/point_light.png"), "texture_diffuse", "res/billboards/point_light.png" };
        // Cubemap.
        displayLoadingMsg("Creating cubemap", &textShader, &window);
        sky.load("res/FortPoint", "posz.jpg", "negz.jpg", "posy.jpg", "negy.jpg", "posx.jpg", "negx.jpg");
        sky.initialize(100);
        // Framebuffer.
        displayLoadingMsg("Creating FBO", &textShader, &window);
        ppFBO = Framebuffer(window.getSize(), 2);
        imguiFBO = Framebuffer(window.getSize());
        ppFBO.quad();
        if (!ppFBO.isComplete()) LOG_ERRR("FBO isn't complete");
        if (!imguiFBO.isComplete()) LOG_ERRR("FBO isn't complete");
        camera.farPlane = 1000;
        sceneWinSize = window.getSize();
        // ImGUI setup.
        LOG_INFO("Initializing ImGui");
        displayLoadingMsg("Initializing ImGui", &textShader, &window);
        FSImGui::Initialize(&window);
        FSImGui::LoadFont("res/fonts/Ubuntu-Regular.ttf", 14, true);
        FSImGui::MD::LoadFonts("res/fonts/Ubuntu-Regular.ttf", "res/fonts/Ubuntu-Bold.ttf", 14, 15.4f);
        LOG_INFO("ImGui ready");
        newsTxtLoaded = StrFromFile("News.md");
        //LuaBridge.
        displayLoadingMsg("Initializing Lua", &textShader, &window);
        using namespace luabridge;
        L = luaL_newstate();
        luaL_openlibs(L);
        lua_pcall(L, 0, 0, 0);

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
        .beginNamespace("firesteel")
            .addConstant("version", GLOBAL_VER.c_str())
            .beginNamespace("log")
                .beginNamespace("f_color")
                    .addConstant("black"  , &cmd_v[0])
                    .addConstant("blue"   , &cmd_v[1])
                    .addConstant("green"  , &cmd_v[2])
                    .addConstant("cyan"   , &cmd_v[3])
                    .addConstant("red"    , &cmd_v[4])
                    .addConstant("purple" , &cmd_v[5])
                    .addConstant("yellow" , &cmd_v[6])
                    .addConstant("gray"   , &cmd_v[7])
                    .addConstant("lblack" , &cmd_v[8])
                    .addConstant("lblue"  , &cmd_v[9])
                    .addConstant("lgreen" , &cmd_v[10])
                    .addConstant("lcyan"  , &cmd_v[11])
                    .addConstant("lred"   , &cmd_v[12])
                    .addConstant("lpurple", &cmd_v[13])
                    .addConstant("lyellow", &cmd_v[14])
                    .addConstant("white"  , &cmd_v[15])
                .endNamespace()
                .beginNamespace("b_color")
                    .addConstant("black"  , &cmd_v[16])
                    .addConstant("blue"   , &cmd_v[17])
                    .addConstant("green"  , &cmd_v[18])
                    .addConstant("cyan"   , &cmd_v[19])
                    .addConstant("red"    , &cmd_v[20])
                    .addConstant("purple" , &cmd_v[21])
                    .addConstant("yellow" , &cmd_v[22])
                    .addConstant("gray"   , &cmd_v[23])
                    .addConstant("lblack" , &cmd_v[23])
                    .addConstant("lblue"  , &cmd_v[24])
                    .addConstant("lgreen" , &cmd_v[25])
                    .addConstant("lcyan"  , &cmd_v[26])
                    .addConstant("lred"   , &cmd_v[27])
                    .addConstant("lpurple", &cmd_v[28])
                    .addConstant("lyellow", &cmd_v[29])
                    .addConstant("white"  , &cmd_v[30])
                .endNamespace()
                .addFunction("info", Log::log_info)
                .addFunction("warn", Log::log_warn)
                .addFunction("error", Log::log_error)
                .addFunction("crit", Log::log_critical)
                .addFunction("custom", Log::log_c)
            .endNamespace()
        .endNamespace();

        luaL_dofile(L, "res/oninit.lua");
        //OpenAL.
        phoneAmbience.init("res/audio/tone.wav", 0.1f, true)->setPostion(10.f, 0.f, 10.f)->play();
        audio.init("res/audio/harbour-port-ambience.wav", 0.2f, true)->play();
        window.setTitle(std::filesystem::current_path().u8string() + " | Firesteel " + GLOBAL_VER);
    }
    virtual void onUpdate() override {
        if (!drawImGUI) if (ppFBO.getSize() != window.getSize()) {
                ppFBO.scale(window.getSize());
                camera.aspect = ppFBO.aspect();
            }
        glViewport(0, 0, static_cast<GLsizei>(ppFBO.getWidth()), static_cast<GLsizei>(ppFBO.getHeight()));
        std::thread GPUThreadT(_gpuThreadTimer);
        /* Input processing */ {
            processInput(&window, window.ptr(), deltaTime);
            audio.setPostion(camera.pos);
            if (glfwGetMouseButton(window.ptr(), 1) != GLFW_PRESS) window.setCursorMode(Window::CUR_NORMAL);
            else window.setCursorMode(Window::CUR_DISABLED);
            ppFBO.bind();
            updateLightInShader(&modelShader);
            window.clearBuffers();
            wireframeEnabled ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        /* Prerender processing */ {
            sLight.position = camera.pos;
            sLight.direction = camera.Forward;
            sky.bind();
            modelShader.enable();
            modelShader.setInt("lightingType", lightingMode);
            modelShader.setInt("DrawMode", drawMode);
            modelShader.setInt("skybox", 11);
            modelShader.setVec3("spotLights[0].position", sLight.position);
            modelShader.setVec3("spotLights[0].direction", sLight.direction);
        }
        glm::mat4 projection = camera.getProjection(clipSpace);
        glm::mat4 view = camera.getView();
        glm::mat4 model = glm::mat4(1.0f);
        std::thread DrawThreadT(_drawThreadTimer);
        /* Skybox */ {
            skyShader.enable();
            skyShader.setInt("DrawMode", drawMode);
            skyShader.setMat4("projection", projection);
            skyShader.setMat4("view", view);
            sky.draw(&skyShader);
        }
        /* Render backpack */ {
            modelShader.enable();
            modelShader.setMat4("projection", projection);
            modelShader.setMat4("view", view);
            modelShader.setVec3("viewPos", camera.pos);
            modelShader.setFloat("material.emissionFactor", 0);
            //city.draw(modelShader);
            backpack.draw(&modelShader);
        }
        /* Render phone booth */ {
            modelShader.setFloat("material.emissionFactor", 5);
            modelShader.setVec3("material.emissionColor", glm::vec3(1));
            phoneBooth.draw(&modelShader);
        }
        /* Render box */ {
            box.transform.Rotation = glm::vec3((float)glfwGetTime() * -10.0f);
            modelShader.setFloat("material.emissionFactor", 1);
            modelShader.setVec3("material.emissionColor", glm::vec3(0.0f, 0.5f, 0.69f));
            box.draw(&modelShader);
        }
        /* Particle system */ {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(5.f, 0.f, 0.f));
            particlesShader.enable();
            particlesShader.setInt("DrawMode", drawMode);
            particlesShader.setMat4("view", view);
            particlesShader.setMat4("projection", projection);
            particlesShader.setMat4("model", model);
            pS.update(deltaTime);
            pS.draw(&particlesShader);
        }
        /* Draw billboard */ {
            quad.transform.Position = glm::vec3(0.f, 5.f, 0.f);
            billTex.bind();
            billboardShader.enable();
            billboardShader.setInt("DrawMode", drawMode);
            billboardShader.setMat4("projection", projection);
            billboardShader.setMat4("view", view);
            billboardShader.setVec3("color", glm::vec3(1));
            billboardShader.setVec2("size", glm::vec2(1.f));
            quad.draw(&billboardShader);
            Texture::unbind();
        }
        /* Draw pseudo-gizmos */ {
            if (displayGizmos) {
                quad.transform.Position = pLight.position;
                glDepthFunc(GL_ALWAYS);
                pointLightBillTex.bind();
                billboardShader.setVec3("color", pLight.diffuse);
                billboardShader.setVec2("size", glm::vec2(0.25f));
                billboardShader.setMat4("model", model);
                quad.draw(&billboardShader);
                Texture::unbind();
                glDepthFunc(GL_LESS);
            }
        }
        /* Draw FBO */ {
            ppFBO.unbind();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            fboShader.enable();
            fboShader.setInt("AAMethod", 1);
            fboShader.setFloat("exposure", 1);
            fboShader.setVec2("screenSize", window.getSize());
            fboShader.setVec3("chromaticOffset", glm::vec3(10));
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ppFBO.getID(0));
            fboShader.setInt("screenTexture", 1);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, ppFBO.getID(1));
            fboShader.setInt("bloomBlur", 2);
            if (drawImGUI) imguiFBO.bind();
            window.clearBuffers();
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
        threadRuntime[1] = false;
        DrawThreadT.join();
        /* Draw UI */ {
            if (drawNativeUI) {
                t.draw(&textShader, std::to_string(fps), ppFBO.getSize(), glm::vec2(8.0f, 568.0f), glm::vec2(1.5f), glm::vec3(0));
                t.draw(&textShader, std::to_string(fps), ppFBO.getSize(), glm::vec2(10.0f, 570.0f), glm::vec2(1.5f), glm::vec3(1, 0, 0));

                t.draw(&textShader, currentDateTime(), ppFBO.getSize(), glm::vec2(8.0f, 553.0f), glm::vec2(1.f), glm::vec3(0));
                t.draw(&textShader, currentDateTime(), ppFBO.getSize(), glm::vec2(10.0f, 555.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));

                t.draw(&textShader, std::string("[V] VSync: ") + (window.getVSync() ? "ON" : "OFF"),
                    ppFBO.getSize(), glm::vec2(8.0f, 538.0f), glm::vec2(1.f), glm::vec3(0));
                t.draw(&textShader, std::string("[V] VSync: ") + (window.getVSync() ? "ON" : "OFF"),
                    ppFBO.getSize(), glm::vec2(10.0f, 540.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));

                t.draw(&textShader, "[Wheel] FOV: " + std::to_string((int)camera.fov),
                    ppFBO.getSize(), glm::vec2(8.0f, 523.0f), glm::vec2(1.f), glm::vec3(0));
                t.draw(&textShader, "[Wheel] FOV: " + std::to_string((int)camera.fov),
                    ppFBO.getSize(), glm::vec2(10.0f, 525.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));

                if (drawSelectMod == 0) {
                    t.draw(&textShader, "[1-0] Shading: " + getDrawModName(),
                        ppFBO.getSize(), glm::vec2(8.0f, 508.0f), glm::vec2(1.f), glm::vec3(0));
                    t.draw(&textShader, "[1-0] Shading: " + getDrawModName(),
                        ppFBO.getSize(), glm::vec2(10.0f, 510.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
                }
                else {
                    t.draw(&textShader, "[1-4] Lighting: " + getLightingTypeName(),
                        ppFBO.getSize(), glm::vec2(8.0f, 508.0f), glm::vec2(1.f), glm::vec3(0));
                    t.draw(&textShader, "[1-4] Lighting: " + getLightingTypeName(),
                        ppFBO.getSize(), glm::vec2(10.0f, 510.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
                }
            }
        }
        unsigned int imguiW=0, imguiH=0;
        /* Draw ImGui */ {
            if (drawImGUI) {
                std::thread ImGuiThreadT(_imguiThreadTimer);
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
                imguiW = static_cast<unsigned int>(ImGui::GetWindowViewport()->Size.x - 1);
                imguiH = static_cast<unsigned int>(ImGui::GetWindowViewport()->Size.y);
                // Upper help menu.
                ImGui::PopStyleVar(1);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
                if (ImGui::BeginMenuBar()) {
                    if (ImGui::BeginMenu(u8"Файл")) {
                        if (ImGui::MenuItem(u8"Сохранить")) {
                            FileDialog fd;
                            fd.filter = "All\0*.*\0FSE Project (*.fse)\0*.fse\0";
                            fd.filter_id = 2;
                            std::string res = fd.save();
                            if (res != "") {
                                StrToFile(res, "Still WIP.");
                                LOG_INFO("Saved project to \"" + res + "\".");
                                didSaveCurPrj = true;
                            }
                        }
                        if (ImGui::MenuItem(u8"Открыть")) {
                            FileDialog fd;
                            fd.filter = "All\0*.*\0FSE Project (*.fse)\0*.fse\0";
                            fd.filter_id = 2;
                            std::string res = fd.open();
                            if (res != "") {
                                LOG_INFO("Opened project to \"" + res + "\".");
                                didSaveCurPrj = true;
                            }
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
                        ImGui::Checkbox(u8"Свет", &lightingOpen);
                        ImGui::Checkbox(u8"Содержимое сцены", &sceneContentViewOpen);
                        ImGui::Checkbox(u8"Редактор объектов", &entityEditorOpen);
                        ImGui::Separator();
                        if (ImGui::MenuItem(u8"Скриншот")) needToScreenShot = AskForScreenShot();
                        if (ImGui::MenuItem(u8"Скриншот Редактора")) needToScreenShotEditor = AskForScreenShot();
                        if (ImGui::MenuItem(u8"Сбросить")) {
                            texViewOpen = false;
                            newsViewOpen = true;
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu(u8"Помощь")) {
                        if (ImGui::MenuItem(u8"Сайт Xanytka Devs")) openURL("http://devs.xanytka.ru/");
                        if (ImGui::MenuItem(u8"Вики FS Editor")) openURL("http://devs.xanytka.ru/wiki/fse");
                        ImGui::Separator();
                        if (ImGui::MenuItem(u8"Сообщить о проблеме")) openURL("https://forms.yandex.ru/u/6751fe9bd04688cc151223e8/");
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu(u8"Тестирование")) {
                        if (ImGui::MenuItem(u8"Переключить Нативный UI")) { drawNativeUI = !drawNativeUI; }
                        if (ImGui::MenuItem(u8"Выключить ImGui")) {
                            drawImGUI = false;
                            LOG_INFO("ImGui rendering sequence disabled.\n    To enable it back... IDK, restart, maybe?");
                        }
                        ImGui::Separator();
                        ImGui::Checkbox(u8"Просмотр текстур", &texViewOpen);
                        ImGui::Checkbox(u8"Статистика потоков", &threadsOpen);
                        ImGui::Checkbox(u8"Консоль разработчика", &consoleDevMode);
                        ImGui::Checkbox(u8"Создавать бэкапы логов", &backupLogs);
                        ImGui::EndMenu();
                    }
                    if (ImGui::Button("Guizmo")) displayGizmos = !displayGizmos;
                    if (currentGizmoMode == ImGuizmo::LOCAL ? ImGui::Button("L") : ImGui::Button("W"))
                        currentGizmoMode == ImGuizmo::LOCAL ? currentGizmoMode = ImGuizmo::WORLD : currentGizmoMode = ImGuizmo::LOCAL;
                    ImGui::EndMenuBar();
                }
                ImGui::End();

                if (newsViewOpen) {
                    ImGui::Begin("News", &newsViewOpen);
                    FSImGui::MD::Text(newsTxtLoaded);
                    ImGui::End();
                }
                if (sceneViewOpen) {
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
                    ImGui::Image((void*)(size_t)imguiFBO.getID(), winSceneSize, ImVec2(0, 1), ImVec2(1, 0));

                    ImGui::PopStyleVar(3);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);

                    // Guizmos.
                    if (displayGizmos) {
                        if (heldEntity) {
                            glm::mat4 curModel = heldEntity->getMatrix();
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
                            heldEntity->transform.Position = float3ToVec3(transl);
                            heldEntity->transform.Rotation = float3ToVec3(rotat);
                            heldEntity->transform.Size = float3ToVec3(scalel);
                        }
                    }
                    ImGui::End();
                }
                if (entityEditorOpen) {
                    ImGui::Begin("Entity Editor", &entityEditorOpen);
                    if (heldEntity) {
                        ImGui::Text("err:naming_not_implemented");
                        ImGui::Separator();
                        FSImGui::DragFloat3(u8"Позиция", &heldEntity->transform.Position);
                        FSImGui::DragFloat3(u8"Вращение", &heldEntity->transform.Rotation);
                        FSImGui::DragFloat3(u8"Размер", &heldEntity->transform.Size);
                        ImGui::BeginDisabled(true);
                        ImGui::InputText(u8"Модель", &heldEntity->path);
                        ImGui::EndDisabled();
                        ImGui::SameLine();
                        if(ImGui::Button("...")) {
                            LOG_INFO("Opening dialog for model selection (Entity:\"err:naming_not_implemented\")");
                            FileDialog fd;
                            fd.filter = "All\0*.*\0OBJ Model (*.obj)\0*.obj\0FBX Model (*.fbx)\0*.fbx\0GLTF Model (*.gltf)\0*.gltf\0";
                            fd.filter_id = 2;
                            std::string modelPath = fd.open();
                            if (modelPath != "") {
                                LOG_INFO("Model at \"" + modelPath + "\" does exist. Exchanging...");
                                heldEntity->clearMeshes();
                                heldEntity->loadFromFile(modelPath);
                            }
                        }
                        ImGui::Separator();
                        if (ImGui::CollapsingHeader(u8"Материал")) {
                            ImGui::Text(u8"Текстуры");
                            for (size_t i = 0; i < heldEntity->texturesLoaded.size(); i++) {
                                ImGui::Text((heldEntity->texturesLoaded[i].type + " ##" + std::to_string(i)).c_str());
                                ImGui::BeginDisabled(true);
                                ImGui::InputText((u8"Путь##" + std::to_string(i)).c_str(), &heldEntity->texturesLoaded[i].path);
                                ImGui::EndDisabled();
                                ImGui::SameLine();
                                if (ImGui::Button(("...##" + std::to_string(i)).c_str())) {
                                    LOG_INFO("Opening dialog for texture selection (Entity:\"err:naming_not_implemented\")");
                                    FileDialog fd;
                                    fd.filter = "All\0*.*\0PNG Image (*.png)\0*.png\0JPEG Image (*.jpg)\0*.jpg\0";
                                    fd.filter_id = 2;
                                    std::string texPath = fd.open();
                                    if (texPath != "") {
                                        LOG_INFO("Texture at \"" + texPath + "\" does exist. Exchanging...");
                                        heldEntity->texturesLoaded[i].remove();
                                        Texture t;
                                        t.ID = TextureFromFile(texPath);
                                        t.type = heldEntity->texturesLoaded[i].type;
                                        t.path = texPath;
                                        heldEntity->texturesLoaded[i] = t;
                                    }
                                }
                                ImGui::Image((void*)(size_t)heldEntity->texturesLoaded[i].ID, ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x));
                            }
                        }
                    }
                    ImGui::End();
                }
                if (sceneContentViewOpen) {
                    ImGui::Begin("Scene Content", &sceneContentViewOpen);
                    if (ImGui::Button("backpack")) heldEntity = &backpack;
                    if (ImGui::Button("box")) heldEntity = &box;
                    if (ImGui::Button("phone booth")) heldEntity = &phoneBooth;
                    ImGui::End();
                }
                if (atmosphereOpen) {
                    ImGui::Begin("Atmosphere", &atmosphereOpen);
                    if (ImGui::CollapsingHeader("Directional Light")) {
                        FSImGui::DragFloat3("Direction", &atmos.directionalLight.direction);
                        ImGui::Separator();
                        FSImGui::ColorEdit3("Ambient", &atmos.directionalLight.ambient);
                        FSImGui::ColorEdit3("Diffuse", &atmos.directionalLight.diffuse);
                        FSImGui::ColorEdit3("Specular", &atmos.directionalLight.specular);
                    }
                    if (ImGui::CollapsingHeader("Fog")) {
                        FSImGui::ColorEdit3("Color", &atmos.fog.color);
                        ImGui::Separator();
                        ImGui::DragFloat("Start", &atmos.fog.start, 0.1f);
                        ImGui::DragFloat("End", &atmos.fog.end, 0.1f);
                        ImGui::DragFloat("Density", &atmos.fog.density, 0.1f);
                        ImGui::DragInt("Equation", &atmos.fog.equation, 1, 0, 2);
                    }
                    if (ImGui::CollapsingHeader("Camera")) {
                        FSImGui::DragFloat3("Position", &camera.pos);
                        FSImGui::DragFloat3("Rotation", &camera.rot);
                        float planes[2] = { camera.nearPlane, camera.farPlane };
                        ImGui::DragFloat2("Planes", planes, 0.1f);
                        ImGui::Checkbox("Is Perspective", &camera.isPerspective);
                        ImGui::DragFloat("Field of View", &camera.fov, 0.01f);
                        ImGui::Checkbox("Override private properties", &overrideAspect);
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("!!! WARNING !!!");
                            ImGui::Text(u8"Если вы не знаете, что делают эти параметры - не трогайте их.");
                            ImGui::EndTooltip();
                        }
                        if (overrideAspect) {
                            ImGui::DragFloat("Aspect", &camera.aspect, 0.01f);
                            ImGui::DragFloat("Clip space", &clipSpace, 0.01f);
                        }
                    }

                    ImGui::End();
                }
                if (soundMixerOpen) {
                    ImGui::Begin("Sound Mixer", &soundMixerOpen);
                    {
                        ImGui::BeginGroup();
                        ImGui::Text("Background");
                        ImGui::BeginDisabled(bgMuted);
                        if (ImGui::VSliderFloat("##BackgroundSlider", ImVec2(40, 200), &alBG.gain, 0.0f, 1.0f, "")) {
                            audio.setGain(alBG);
                        }
                        std::string val = std::to_string(alBG.gain);
                        val = val.substr(0, 5);
                        ImGui::Text(val.c_str());
                        if (ImGui::Knob("##BackgroundKnob", &alBG.pitch, 0.01f, 5.0f, 0.0f, "%.3f",
                            ImGuiKnobVariant_Wiper, 0.0f, ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput)) {
                            audio.setPitch(alBG);
                        }
                        ImGui::EndDisabled();
                        if (ImGui::Button("M##Background")) {
                            if (!bgMuted) preMuteGain = alBG.gain;
                            bgMuted = !bgMuted;
                            if (bgMuted) alBG.gain = 0;
                            else alBG.gain = preMuteGain;
                            audio.setGain(alBG);
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
                            phoneAmbience.setGain(alSFX);
                        }
                        std::string val = std::to_string(alSFX.gain);
                        val = val.substr(0, 5);
                        ImGui::Text(val.c_str());
                        if (ImGui::Knob("##SFXKnob", &alSFX.pitch, 0.01f, 5.0f, 0.0f, "%.3f",
                            ImGuiKnobVariant_Wiper, 0.0f, ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput)) {
                            phoneAmbience.setPitch(alSFX);
                        }
                        ImGui::EndDisabled();
                        if (ImGui::Button("M##SFX")) {
                            if (!sfxMuted) preMuteGain = alSFX.gain;
                            sfxMuted = !sfxMuted;
                            if (sfxMuted) alSFX.gain = 0;
                            else alSFX.gain = preMuteGain;
                            phoneAmbience.setGain(alSFX);
                        }
                        val = std::to_string(alSFX.pitch);
                        val = val.substr(0, 5);
                        ImGui::Text(val.c_str());
                        ImGui::EndGroup();
                    }
                    ImGui::End();
                }
                if (lightingOpen) {
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
                if (undoHistoryOpen) {
                    ImGui::Begin("Undo History", &undoHistoryOpen);
                    for (size_t i = 0; i < 4; i++)
                    {
                        ImGui::BeginGroup();
                        ImGui::Text(acts[i]);
                        ImGui::EndGroup();
                    }
                    ImGui::End();
                }

                if (texViewOpen) {
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
                        ImGui::Image((void*)(size_t)i, ImVec2(100, 100));
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
                if (fullTexViewOpen) {
                    ImGui::Begin("Full Texture Viewer", &fullTexViewOpen);
                    glActiveTexture(GL_TEXTURE10);
                    glBindTexture(GL_TEXTURE_2D, texID);
                    ImGui::Image((void*)(size_t)texID, ImVec2(ImGui::GetWindowWidth() * 0.9f, ImGui::GetWindowHeight() * 0.9f));
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE0);
                    ImGui::End();
                }
                if (consoleOpen) {
                    ImGui::Begin("Console", &consoleOpen);
                    if (consoleDevMode) {
                        if (ImGui::Button(u8"Ошибки")) switchedToDevConsole = false;
                        ImGui::SameLine();
                        if (ImGui::Button(u8"Консоль")) switchedToDevConsole = true;
                    } else switchedToDevConsole = false;
                    if(consoleDevMode && switchedToDevConsole) {
                        ImGui::Text(consoleLog.c_str());
                        ImGuiInputTextFlags inputTextFlags =
                            ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCharFilter |
                            ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways;
                        ImGui::PushItemWidth(-ImGui::GetStyle().ItemSpacing.x * 7);
                        if(ImGui::InputText("##dev_input", &consoleInput, inputTextFlags, nullptr, this)) {
                            LOG_INFO("Executed cmd '" + consoleInput + "'");
                            consoleLog += "> " + consoleInput + "\n";
                            std::vector<std::string> consoleInputs = StrSplit(consoleInput, ' ');
                            if(consoleInputs[0] == "help") {
                                const char* hlpstr = "\
- help\n All available commands.\n\n\
- lua\n Execute following Lua code (output to CMD, not this console).\n\n\
- luaF\n Execute Lua code from following file (output to CMD, not this console).\n\n\
- exit\n Exits out of the app.\n\n\
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
                            } else {
                                const char* invstr = " Invalid command.\n";
                                consoleLog += invstr;
                                LOG(invstr);
                            }
                            consoleInput.clear();
                        }
                        ImGui::PopItemWidth();
                    } else {
                        ImGui::Text(u8"Нет ошибок");
                    }
                    ImGui::End();
                }

                if (drawImGUI) {
                    threadRuntime[0] = false;
                    GPUThreadT.join();
                }
                threadRuntime[2] = false;
                ImGuiThreadT.join();
                if (threadsOpen) {
                    ImGui::Begin("Threads", &threadsOpen);
                    ImGui::Text(("FPS: " + std::to_string(fps)).c_str());
                    ImGui::Text(("Delta time: " + std::to_string(deltaTime)).c_str());
                    ImGui::Separator();
                    ImGui::Text(("GPU Thread: " + std::to_string(_gpuThreadTime)).c_str());
                    ImGui::Text(("Draw Thread: " + std::to_string(_drawThreadTime)).c_str());
                    ImGui::Text(("ImGui Thread: " + std::to_string(_imguiThreadTime)).c_str());
                    ImGui::End();
                }

                // Rendering
                FSImGui::Render(&window);
            }
            if (!drawImGUI) {
                threadRuntime[0] = false;
                GPUThreadT.join();
            }
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
    }
    virtual void onShutdown() override {
        loadedW = window.getWidth();
        loadedH = window.getHeight();
        FSImGui::Shutdown();
        LOG_INFO("ImGui terminated");
        // OpenAL.
        phoneAmbience.remove();
        audio.remove();
        FSOAL::deinitialize();
    }
};

static void saveConfig() {
    nlohmann::json txt;
    if(!newsViewOpen) txt["openedNews"] = GLOBAL_VER;
    else txt["openedNews"] = "false";

    txt["window"]["width"] = loadedW;
    txt["window"]["height"] = loadedH;

    txt["dev"]["consoleDevMode"] = consoleDevMode;
    txt["dev"]["backupLogs"] = backupLogs;

    txt["layout"]["console"] = consoleOpen;
    txt["layout"]["soundMixer"] = soundMixerOpen;
    txt["layout"]["sceneView"] = sceneViewOpen;
    txt["layout"]["metrics"] = threadsOpen;
    txt["layout"]["lighting"] = lightingOpen;
    txt["layout"]["atmosphere"] = atmosphereOpen;
    txt["layout"]["entityEditor"] = entityEditorOpen;
    txt["layout"]["sceneContentView"] = sceneContentViewOpen;
    txt["layout"]["undoHistory"] = undoHistoryOpen;

    std::ofstream o(".fsecfg");
    o << std::setw(4) << txt << std::endl;
}
static void loadConfig() {
    if(!std::filesystem::exists(".fsecfg")) return;
    std::ifstream ifs(".fsecfg");
    nlohmann::json txt = nlohmann::json::parse(ifs);
    if(!txt["openedNews"].is_null()) {
        if (txt["openedNews"] == "0.2.0.4") newsViewOpen = false;
        else newsViewOpen = true;
    }
    if(!txt["window"].is_null()) {
        loadedW = txt["window"]["width"];
        loadedH = txt["window"]["height"];
    }
    if(!txt["dev"].is_null()) {
        if(!txt["dev"]["consoleDevMode"].is_null()) consoleDevMode = txt["dev"]["consoleDevMode"];
        if(!txt["dev"]["backupLogs"].is_null())     backupLogs = txt["dev"]["backupLogs"];
    }
    if(!txt["layout"].is_null()) {
        txt = txt.at("layout");
        if(!txt["console"].is_null())            consoleOpen = txt["console"];
        if(!txt["soundMixer"].is_null())         soundMixerOpen = txt["soundMixer"];
        if(!txt["sceneView"].is_null())          sceneViewOpen = txt["sceneView"];
        if(!txt["metrics"].is_null())            threadsOpen = txt["metrics"];
        if(!txt["lighting"].is_null())           lightingOpen = txt["lighting"];
        if(!txt["atmosphere"].is_null())         atmosphereOpen = txt["atmosphere"];
        if(!txt["entityEditor"].is_null())       entityEditorOpen = txt["entityEditor"];
        if(!txt["sceneContentView"].is_null())   sceneContentViewOpen = txt["sceneContentView"];
        if(!txt["undoHistory"].is_null())        undoHistoryOpen = txt["undoHistory"];
    }
}

int main() {
    EditorApp app{};
    loadConfig();
    int r = app.start(("Firesteel " + GLOBAL_VER).c_str(), loadedW, loadedH, WS_NORMAL);
    LOG_INFO("Shutting down Firesteel App.");
    if(backupLogs) Log::destroyFileLogger();
    saveConfig();
    return r;
}

static std::string getDrawModName() {
    switch (drawMode) {
    case 0:
        return "Default";
    case 1:
        return "Depth";
    case 2:
        return "Specular";
    case 3:
        return "Normal TS";
    case 4:
        return "Normal WS";
    case 5:
        return "Normal";
    case 6:
        return "Tangents";
    case 7:
        return "Bitangents";
    case 8:
        return "Emission";
    case 9:
        return "Skybox Refractions";
    default:
        return "unknown";
    }
}
static std::string getLightingTypeName() {
    switch (lightingMode) {
    case 0:
        return "No lighting";
    case 1:
        return "Phong";
    case 2:
        return "Blinn-Phong";
    case 3:
        return "Phong and Blinn-Phong";
    default:
        return "unknown";
    }
}

float speed_mult = 2.f;
void processInput(Window* tWin, GLFWwindow* tPtr, float tDeltaTime) {
    if (glfwGetKey(tPtr, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        tWin->close();

    if (glfwGetKey(tPtr, GLFW_KEY_E) == GLFW_PRESS) currentGizmoOperation = ImGuizmo::TRANSLATE;
    if (glfwGetKey(tPtr, GLFW_KEY_R) == GLFW_PRESS) currentGizmoOperation = ImGuizmo::ROTATE;
    if (glfwGetKey(tPtr, GLFW_KEY_T) == GLFW_PRESS) currentGizmoOperation = ImGuizmo::SCALE;

    if (glfwGetKey(tPtr, GLFW_KEY_L) == GLFW_PRESS) currentGizmoMode = ImGuizmo::LOCAL;
    if (glfwGetKey(tPtr, GLFW_KEY_K) == GLFW_PRESS) currentGizmoMode = ImGuizmo::WORLD;

    if (glfwGetMouseButton(tPtr, 1) != GLFW_PRESS) return;

    float velocity = 2.5f * tDeltaTime;
    if (glfwGetKey(tPtr, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        velocity *= speed_mult;
        speed_mult += 0.005f;
    }
    else speed_mult = 2.f;

    if (glfwGetKey(tPtr, GLFW_KEY_M) == GLFW_PRESS)
        tWin->toggleVSync();

    if (glfwGetKey(tPtr, GLFW_KEY_W) == GLFW_PRESS)
        camera.pos += camera.Forward * velocity;
    if (glfwGetKey(tPtr, GLFW_KEY_S) == GLFW_PRESS)
        camera.pos -= camera.Forward * velocity;
    if (glfwGetKey(tPtr, GLFW_KEY_A) == GLFW_PRESS)
        camera.pos -= camera.Right * velocity;
    if (glfwGetKey(tPtr, GLFW_KEY_D) == GLFW_PRESS)
        camera.pos += camera.Right * velocity;

    if (glfwGetKey(tPtr, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.pos.y += velocity;
    if (glfwGetKey(tPtr, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.pos.y -= velocity;

    FSOAL::Listener::setPosition(camera.pos);
    FSOAL::Listener::setRotation(camera.Forward, camera.Up);

    if (glfwGetKey(tPtr, GLFW_KEY_L) == GLFW_PRESS) drawSelectMod = 1;
    if (glfwGetKey(tPtr, GLFW_KEY_H) == GLFW_PRESS) drawSelectMod = 0;
    if (drawSelectMod == 0) {
        lightingMode = 3;
        if (glfwGetKey(tPtr, GLFW_KEY_1) == GLFW_PRESS) { drawMode = 0; }
        if (glfwGetKey(tPtr, GLFW_KEY_2) == GLFW_PRESS) { drawMode = 2; }
        if (glfwGetKey(tPtr, GLFW_KEY_3) == GLFW_PRESS) { drawMode = 3; }
        if (glfwGetKey(tPtr, GLFW_KEY_4) == GLFW_PRESS) { drawMode = 4; }
        if (glfwGetKey(tPtr, GLFW_KEY_5) == GLFW_PRESS) { drawMode = 5; }
        if (glfwGetKey(tPtr, GLFW_KEY_6) == GLFW_PRESS) { drawMode = 6; }
        if (glfwGetKey(tPtr, GLFW_KEY_7) == GLFW_PRESS) { drawMode = 7; }
        if (glfwGetKey(tPtr, GLFW_KEY_8) == GLFW_PRESS) { drawMode = 8; }
        if (glfwGetKey(tPtr, GLFW_KEY_9) == GLFW_PRESS) { drawMode = 9; }
        if (glfwGetKey(tPtr, GLFW_KEY_0) == GLFW_PRESS) { drawMode = 1; }
    }
    if (drawSelectMod == 1) {
        drawMode = 0;
        if (glfwGetKey(tPtr, GLFW_KEY_1) == GLFW_PRESS) { lightingMode = 0; }
        if (glfwGetKey(tPtr, GLFW_KEY_2) == GLFW_PRESS) { lightingMode = 1; }
        if (glfwGetKey(tPtr, GLFW_KEY_3) == GLFW_PRESS) { lightingMode = 2; }
        if (glfwGetKey(tPtr, GLFW_KEY_4) == GLFW_PRESS) { lightingMode = 3; }
    }    

    if (glfwGetKey(tPtr, GLFW_KEY_V) == GLFW_PRESS) { wireframeEnabled = !wireframeEnabled; }
    if (glfwGetKey(tPtr, GLFW_KEY_F) == GLFW_PRESS) { sLightEnabled = !sLightEnabled; }
}
float MouseSensitivity = 0.1f;
void mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    if(glfwGetMouseButton(window, 1) != GLFW_PRESS) { firstMouse = true; return; }
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;
    camera.rot.z += xoffset;
    camera.rot.y += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (camera.rot.y > 89.0f)
        camera.rot.y = 89.0f;
    if (camera.rot.y < -89.0f)
        camera.rot.y = -89.0f;
    if (camera.rot.z >= 360.0f)
        camera.rot.z -= 360.0f;
    if (camera.rot.z <= -360.0f)
        camera.rot.z += 360.0f;

    // update Front, Right and Up Vectors using the updated Euler angles
    camera.update();
}
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if(glfwGetMouseButton(window, 1) != GLFW_PRESS) return;
    camera.fov -= static_cast<float>(yoffset);
    if (camera.fov < 1.0f)
        camera.fov = 1.0f;
    if (camera.fov > 100.0f)
        camera.fov = 100.0f;
}
static void displayLoadingMsg(std::string t_Msg, Shader* t_Shader, Window* t_Window) {
    t_Window->pollEvents();
    t_Window->clearBuffers();
    t.draw(t_Shader, "App is loading", t_Window->getSize(),
        glm::vec2(t_Window->getWidth() / 3, t_Window->getHeight() / 2), glm::vec2(1.5f), glm::vec3(0.75f, 0.5f, 0.f));
    t.draw(t_Shader, t_Msg, t_Window->getSize(),
        glm::vec2(10, 15), glm::vec2(1.f), glm::vec3(1));
    t_Window->swapBuffers();
}