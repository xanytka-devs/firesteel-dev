#include "engine/include/2d/text.hpp"
#include "engine/include/camera.hpp"
#include "engine/include/cubemap.hpp"
#include "engine/include/entity.hpp"
#include "engine/include/fbo.hpp"
#include "engine/include/light.hpp"
#include "engine/include/atmosphere.hpp"
#include "engine/include/particles.hpp"
#include "engine/include/app.hpp"
#include "include/embeded.hpp"
using namespace Firesteel;

#include <stdio.h>
#include <time.h>
#include "include/imgui/markdown.hpp"
#include <../external/imgui/imgui-knobs.h>
#include <../external/imgui/ImGuizmo.h>
#include "engine/include/audio/source.hpp"
#include "engine/include/audio/listener.hpp"
#include <glm/gtc/type_ptr.hpp>
#define _GLIBCXX_USE_NANOSLEEP
#include <thread>
#include "../engine/include/utils/json.hpp"
#include "include/undo_buffer.hpp"
#define __STDC_LIB_EXT1__
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../engine/include/utils/stb_image_write.hpp"

extern "C" {
# include "engine/external/lua/include/lua.h"
# include "engine/external/lua/include/lauxlib.h"
# include "engine/external/lua/include/lualib.h"
}
#include "engine/external/lua/LuaBridge/LuaBridge/LuaBridge.h"
#include "include/editor_object.hpp"

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
float preMuteGain = 0.0f;

bool shdrParamMaterialGenPreview = false;
bool threadsOpen = true;
bool texViewOpen = false;
bool fullTexViewOpen = false;
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
const std::string GLOBAL_VER = "0.2.0.6";

float gamma = 2.2f;
float shaderContrast = 1;
float shaderSaturation = 0;
float shaderHue = 0;
bool autoExposure = false;
float exposure = 1.0f, exposureRangeMin = 0.3f, exposureRangeMax = 2.5f, exposureMultiplyer = 1.1f;
bool showBloomSampler = false;
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

EditorObject* heldEntity;
ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;
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

using namespace luabridge;
class EditorApp : public App {
    std::vector<EditorObject> entities;
    std::vector<Material> materialReg;

    Texture billTex, pointLightBillTex;
    Framebuffer ppFBO, imguiFBO;
    Shader fboShader, defaultShader;
    ParticleSystem pS = ParticleSystem(glm::vec3(0, 0, 0), 500, "res/particle.png", false);
    Cubemap sky;

    FSOAL::Source phoneAmbience;
    FSOAL::AudioLayer alSFX{};
    FSOAL::Source audio;
    FSOAL::AudioLayer alBG{};
    std::string newsTxtLoaded = "";

    ActionRegistry actions;
    lua_State* L = nullptr;

    std::filesystem::path lastPath;
    // Reloads all default shaders.
    void reloadShaders() {
        LOG_INFO("'Shader reload' sequence initialized...");
        LOG_INFO("Reloading base shaders");
        for (size_t i = 0; i < materialReg.size(); i++) {
            materialReg[i].reload();
            if(!materialReg[i].isLoaded()) materialReg[4].shader = defaultShader;
        }
        fboShader = Shader("res/fbo.vs", "res/fbo.fs");
        LOG_INFO("Setting parameters");
        materialReg[4].enable();
        materialReg[4].setFloat("material.normalStrength", 1.f);
        materialReg[4].setFloat("material.shininess", 64);
        materialReg[4].setFloat("material.skyboxRefraction", 1.f / 1.52f);
        materialReg[4].setFloat("material.skyboxRefractionStrength", 0.0125f);
        LOG_INFO("'Shader reload' sequence has been completed");
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

    virtual void onInitialize() override {
        window.setIconFromMemory(emb::img_fse_logo, sizeof(emb::img_fse_logo)/ sizeof(*emb::img_fse_logo));
        window.setClearColor(glm::vec3(0.0055f, 0.002f, 0.f));
        // Getting ready text renderer.
        defaultShader = Shader(emb::shd_vrt_default, emb::shd_frg_default, false, "");
        materialReg.push_back(Material().load("res/mats/text.material.json"));
        TextRenderer::initialize();
        t.loadFont("res/fonts/vgasysr.ttf", 16);
        // OpenGL setup.
        displayLoadingMsg("Initializing OpenGL", &materialReg[0].shader, &window);
        glfwSetCursorPosCallback(window.ptr(), mouseCallback);
        glfwSetScrollCallback(window.ptr(), scrollCallback);
        // OpenAL setup.
        displayLoadingMsg("Initializing OpenAL", &materialReg[0].shader, &window);
        FSOAL::initialize();
        audio.init("res/audio/elevator-music.mp3", 0.2f, true)->play();
        LOG_INFO("OpenAL context created:");
        LOG_INFO(std::string("	Device: ") + alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER));
        // Shaders.
        displayLoadingMsg("Compiling shaders", &materialReg[0].shader, &window);
        materialReg.push_back(Material().load("res/mats/billboard.material.json"));
        materialReg.push_back(Material().load("res/mats/particle.material.json"));
        materialReg.push_back(Material().load("res/mats/skybox.material.json"));
        materialReg.push_back(Material().load("res/mats/lit_3d.material.json"));
        reloadShaders();
        pS.init();
        // Load models.
        //displayLoadingMsg("Loading city", &textShader, &window);
        //Transform city("res/city/scene.gltf");
        displayLoadingMsg("Loading backpack", &materialReg[0].shader, &window);
        entities.push_back(EditorObject{ "Backpack", Entity("res\\backpack\\backpack.obj", glm::vec3(0.f, 0.f, -1.f), glm::vec3(0), glm::vec3(0.5)), materialReg[4] });
        displayLoadingMsg("Loading quad", &materialReg[0].shader, &window);
        entities.push_back(EditorObject{ "Quad", Entity("res\\primitives\\quad.obj"), materialReg[1] });
        displayLoadingMsg("Loading box", &materialReg[0].shader, &window);
        entities.push_back(EditorObject{ "Box", Entity("res\\box\\box.obj", glm::vec3(-2.f, 0.f, 1.f)), materialReg[4] });
        displayLoadingMsg("Loading phone booth", &materialReg[0].shader, &window);
        entities.push_back(EditorObject{ "Phone booth", Entity("res\\phone-booth\\X.obj", glm::vec3(10.f, 0.f, 10.f), glm::vec3(0), glm::vec3(0.5f)), materialReg[4] });
        billTex = Texture{ TextureFromFile("res/yeah.png"), "texture_diffuse", "res/yeah.png" };
        pointLightBillTex = Texture{ TextureFromFile("res/billboards/point_light.png"), "texture_diffuse", "res/billboards/point_light.png" };
        // Cubemap.
        displayLoadingMsg("Creating cubemap", &materialReg[0].shader, &window);
        sky.load("res/FortPoint", "posz.jpg", "negz.jpg", "posy.jpg", "negy.jpg", "posx.jpg", "negx.jpg");
        sky.initialize(100);
        // Framebuffer.
        displayLoadingMsg("Creating FBO", &materialReg[0].shader, &window);
        ppFBO = Framebuffer(window.getSize(), 2);
        imguiFBO = Framebuffer(window.getSize());
        ppFBO.quad();
        if (!ppFBO.isComplete()) LOG_ERRR("FBO isn't complete");
        if (!imguiFBO.isComplete()) LOG_ERRR("FBO isn't complete");
        camera.farPlane = 1000;
        sceneWinSize = window.getSize();
        // ImGUI setup.
        LOG_INFO("Initializing ImGui");
        displayLoadingMsg("Initializing ImGui", &materialReg[0].shader, &window);
        FSImGui::Initialize(&window);
        FSImGui::LoadFont("res/fonts/Ubuntu-Regular.ttf", 14, true);
        FSImGui::MD::LoadFonts("res/fonts/Ubuntu-Regular.ttf", "res/fonts/Ubuntu-Bold.ttf", 14, 15.4f);
        LOG_INFO("ImGui ready");
        newsTxtLoaded = StrFromFile("News.md");
        //LuaBridge.
        displayLoadingMsg("Initializing Lua", &materialReg[0].shader, &window);
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
        }
        luaL_dofile(L, "res/scripts/oninit.lua");
        luaL_dofile(L, "res/scripts/components/rotator.lua");
        getGlobal(L, "onstart")();
        //OpenAL.
        phoneAmbience.init("res/audio/tone.wav", 0.1f, true)->setPostion(10.f, 0.f, 10.f)->play();
        audio.init("res/audio/harbour-port-ambience.mp3", 0.2f, true)->play();
        lastPath=std::filesystem::current_path();
        window.setClearColor(glm::vec3(0));
        window.setTitle((didSaveCurPrj?"":"*") + lastPath.u8string() + " | Firesteel " + GLOBAL_VER);
    }
    virtual void onUpdate() override {
        if (!drawImGUI) if (ppFBO.getSize() != window.getSize()) {
                ppFBO.scale(window.getSize());
                camera.aspect = ppFBO.aspect();
            }
        glViewport(0, 0, static_cast<GLsizei>(ppFBO.getWidth()), static_cast<GLsizei>(ppFBO.getHeight()));
        glDisable(GL_FRAMEBUFFER_SRGB);
        std::thread GPUThreadT(_gpuThreadTimer);
        /* Input processing */ {
            processInput(&window, window.ptr(), deltaTime);
            audio.setPostion(camera.pos);
            if (glfwGetMouseButton(window.ptr(), 1) != GLFW_PRESS) window.setCursorMode(Window::CUR_NORMAL);
            else window.setCursorMode(Window::CUR_DISABLED);
            ppFBO.bind();
            updateLightInShader(&materialReg[4].shader);
            window.clearBuffers();
            wireframeEnabled ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        glm::mat4 projection = camera.getProjection(clipSpace);
        glm::mat4 view = camera.getView();
        glm::mat4 model = glm::mat4(1.0f);
        std::thread DrawThreadT(_drawThreadTimer);
        /* Prerender processing */ {
            sLight.position = camera.pos;
            sLight.direction = camera.Forward;
            for (size_t i = 0; i < materialReg.size(); i++)
            {
                materialReg[i].enable();
                materialReg[i].setMat4("projection", projection);
                materialReg[i].setMat4("view", view);
                materialReg[i].setVec3("viewPos", camera.pos);
                materialReg[i].setInt("drawMode", drawMode);
                materialReg[i].setInt("time", (float)glfwGetTime());
            }
            sky.bind();
            materialReg[4].enable();
            materialReg[4].setInt("lightingType", lightingMode);
            materialReg[4].setInt("skybox", 11);
            materialReg[4].setVec3("spotLights[0].position", sLight.position);
            materialReg[4].setVec3("spotLights[0].direction", sLight.direction);
            defaultShader.enable();
            defaultShader.setVec2("resolution", ppFBO.getSize());
        }
        /* Skybox */ {
            sky.draw(&materialReg[3].shader);
        }
        /* Draw billboard */ {
            entities[1].entity.transform.Position = glm::vec3(0.f, 5.f, 0.f);
            billTex.bind();
            materialReg[1].enable();
            materialReg[1].setVec3("color", glm::vec3(1));
            materialReg[1].setVec2("size", glm::vec2(1.f));
            entities[1].draw();
            Texture::unbind();
        }
        /* Draw pseudo-gizmos */ {
            if (displayGizmos) {
                entities[1].entity.transform.Position = pLight.position;
                glDepthFunc(GL_ALWAYS);
                pointLightBillTex.bind();
                materialReg[1].setVec3("color", pLight.diffuse);
                materialReg[1].setVec2("size", glm::vec2(0.25f));
                materialReg[1].setMat4("model", model);
                entities[1].draw();
                Texture::unbind();
                glDepthFunc(GL_LESS);
            }
        }
        /* Particle system */ {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(5.f, 0.f, 0.f));
            materialReg[3].enable();
            materialReg[3].setMat4("model", model);
            pS.update(deltaTime);
            pS.draw(&materialReg[3].shader);
        }
        /* Render backpack */ {
            materialReg[4].enable();
            materialReg[4].setFloat("material.emissionFactor", 2.5f);
            //city.draw(modelShader);
            entities[0].draw();
        }
        /* Render box */ {
            entities[2].entity.transform.Rotation = glm::vec3((float)glfwGetTime() * -10.0f);
            materialReg[4].setVec3("material.emissionColor", glm::vec3(0.0f, 0.5f, 0.69f));
            entities[2].draw();
        }
        /* Render phone booth */ {
            materialReg[4].enable();
            materialReg[4].setVec3("material.emissionColor", glm::vec3(1));
            entities[3].draw();
        }
        /* Draw FBO */ {
            ppFBO.unbind();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            fboShader.enable();
            fboShader.setBool("showBloomSampler", showBloomSampler);
            fboShader.setBool("bloom", bloom);
            fboShader.setBool("invertColors", invertColors);
            fboShader.setBool("vigiette", vigiette);
            fboShader.setBool("useKernel", useShaderKernel);
            if(useShaderKernel) for (size_t i = 0; i < shaderKernelSize; i++)
                fboShader.setFloat("shaderKernel[" + std::to_string(i) + "]", shaderKernel[i]);
            fboShader.setInt("AAMethod", isAAEnabled?1:0);
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
            if (drawImGUI) imguiFBO.bind();
            window.clearBuffers();
            if(autoExposure) {
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
        threadRuntime[1] = false;
        DrawThreadT.join();
        /* Draw UI */ {
            if (drawNativeUI) {
                t.draw(&materialReg[0].shader, std::to_string(fps), ppFBO.getSize(), glm::vec2(8.0f, 568.0f), glm::vec2(1.5f), glm::vec3(0));
                t.draw(&materialReg[0].shader, std::to_string(fps), ppFBO.getSize(), glm::vec2(10.0f, 570.0f), glm::vec2(1.5f), glm::vec3(1, 0, 0));

                t.draw(&materialReg[0].shader, currentDateTime(), ppFBO.getSize(), glm::vec2(8.0f, 553.0f), glm::vec2(1.f), glm::vec3(0));
                t.draw(&materialReg[0].shader, currentDateTime(), ppFBO.getSize(), glm::vec2(10.0f, 555.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));

                t.draw(&materialReg[0].shader, std::string("[V] VSync: ") + (window.getVSync() ? "ON" : "OFF"),
                    ppFBO.getSize(), glm::vec2(8.0f, 538.0f), glm::vec2(1.f), glm::vec3(0));
                t.draw(&materialReg[0].shader, std::string("[V] VSync: ") + (window.getVSync() ? "ON" : "OFF"),
                    ppFBO.getSize(), glm::vec2(10.0f, 540.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));

                t.draw(&materialReg[0].shader, "[Wheel] FOV: " + std::to_string((int)camera.fov),
                    ppFBO.getSize(), glm::vec2(8.0f, 523.0f), glm::vec2(1.f), glm::vec3(0));
                t.draw(&materialReg[0].shader, "[Wheel] FOV: " + std::to_string((int)camera.fov),
                    ppFBO.getSize(), glm::vec2(10.0f, 525.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));

                if (drawSelectMod == 0) {
                    t.draw(&materialReg[0].shader, "[1-0] Shading: " + getDrawModName(),
                        ppFBO.getSize(), glm::vec2(8.0f, 508.0f), glm::vec2(1.f), glm::vec3(0));
                    t.draw(&materialReg[0].shader, "[1-0] Shading: " + getDrawModName(),
                        ppFBO.getSize(), glm::vec2(10.0f, 510.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
                }
                else {
                    t.draw(&materialReg[0].shader, "[1-4] Lighting: " + getLightingTypeName(),
                        ppFBO.getSize(), glm::vec2(8.0f, 508.0f), glm::vec2(1.f), glm::vec3(0));
                    t.draw(&materialReg[0].shader, "[1-4] Lighting: " + getLightingTypeName(),
                        ppFBO.getSize(), glm::vec2(10.0f, 510.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
                }
            }
        }
        unsigned int imguiW=0, imguiH=0;
        /* Draw ImGui */ {
            if (drawImGUI) {
                //glEnable(GL_FRAMEBUFFER_SRGB);
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
                imguiW = static_cast<unsigned int>(ImGui::GetWindowViewport()->Size.x - 2);
                imguiH = static_cast<unsigned int>(ImGui::GetWindowViewport()->Size.y - 1);
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
                        std::filesystem::current_path(lastPath);
                        if (ImGui::MenuItem(u8"Сбросить")) {
                            texViewOpen = false;
                            newsViewOpen = true;
                        }
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
                        if(ImGui::Button(u8"Перезагрузить шейдеры")) reloadShaders();
                        ImGui::Checkbox("Wireframe", &wireframeEnabled);
                        ImGui::Checkbox(u8"Показать текстуру свечения", &showBloomSampler);
                        ImGui::Separator();
                        ImGui::Checkbox(u8"Генерация параметров шейдеров", &shdrParamMaterialGenPreview);
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text(u8"Экспериментальная функция");
                            ImGui::Text(u8"Может резко снизить FPS при открытии параметров материалов в редакторе существ.");
                            ImGui::EndTooltip();
                        }
                        ImGui::Checkbox(u8"Консоль разработчика", &consoleDevMode);
                        ImGui::Checkbox(u8"Просмотр текстур", &texViewOpen);
                        ImGui::Checkbox(u8"Создавать бэкапы логов", &backupLogs);
                        ImGui::Checkbox(u8"Статистика потоков", &threadsOpen);
                        ImGui::Separator();
                        if (ImGui::MenuItem(u8"Переключить Нативный UI")) { drawNativeUI = !drawNativeUI; }
                        if (ImGui::MenuItem(u8"Выключить ImGui")) {
                            drawImGUI = false;
                            LOG_INFO("ImGui rendering sequence disabled.\n    To enable it back... IDK, restart, maybe?");
                        }
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
                if (infoPanelOpen) {
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
                if (entityEditorOpen) {
                    ImGui::Begin("Entity Editor", &entityEditorOpen);
                    if (heldEntity) {
                        ImGui::InputText("Name", &heldEntity->name);
                        if(ImGui::CollapsingHeader(u8"Положение", ImGuiTreeNodeFlags_DefaultOpen)) {
                            FSImGui::DragFloat3(u8"Позиция", &heldEntity->entity.transform.Position);
                            FSImGui::DragFloat3(u8"Вращение", &heldEntity->entity.transform.Rotation);
                            FSImGui::DragFloat3(u8"Размер", &heldEntity->entity.transform.Size);
                        }
                        if(heldEntity->entity.hasModel()) if(ImGui::CollapsingHeader(u8"Модель", ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::Text(heldEntity->entity.model.path.c_str());
                            ImGui::SameLine();
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
                            if (ImGui::Button("X")) heldEntity->entity.clearMeshes();
                            if (ImGui::CollapsingHeader(u8"Материал")) {
                                Material mat = heldEntity->material;
                                ImGui::Text(mat.name.c_str());
                                ImGui::Text(mat.path.c_str());
                                ImGui::SameLine();
                                if(ImGui::Button("...##shdr_exchange")) {
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
                                                mat.setInt(name, paramF);
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
                        }
                    }
                    ImGui::End();
                }
                if (sceneContentViewOpen) {
                    ImGui::Begin("Scene Content", &sceneContentViewOpen);
                    for (size_t i = 0; i < entities.size(); i++)
                        if(ImGui::Button(entities[i].name.c_str())) heldEntity = &entities[i];
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
                    if(ImGui::CollapsingHeader("Post-processing")) {
                        ImGui::DragFloat("Gamma", &gamma, 0.05f);
                        if(gamma<0.1f) gamma = 0.1f;
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
                        if(autoExposure) {
                            FSImGui::DragLFloat2("Exposure range", exposureRangeMin, exposureRangeMax);
                            ImGui::DragFloat("Exposure multiplyer", &exposureMultiplyer);
                        }
                        if (exposure < 0) exposure = 0;
                        ImGui::Separator();

                        ImGui::Checkbox("Bloom", &bloom);
                        if(bloom) ImGui::DragFloat("Bloom weight", &bloomWeight, 0.1f);
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
                        if(useShaderKernel) {
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
                if (fullTexViewOpen) {
                    ImGui::Begin("Full Texture Viewer", &fullTexViewOpen);
                    glActiveTexture(GL_TEXTURE10);
                    glBindTexture(GL_TEXTURE_2D, texID);
                    ImGui::Image((ImTextureID)(size_t)texID, ImVec2(ImGui::GetWindowWidth() * 0.9f, ImGui::GetWindowHeight() * 0.9f));
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
                                } else if(consoleInputs[1] == "OpenGL") {
                                    std::string exts = gimmeOGLExtencions();
                                    consoleLog += exts;
                                    LOG(exts);
                                } else if(consoleInputs[1] == "Vulkan") {
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
        if (glfwGetKey(window.ptr(), GLFW_KEY_F5) == GLFW_PRESS) reloadShaders();
    }
    virtual void onShutdown() override {
        getGlobal(L, "onend")();
        // ImGui
        loadedW = window.getWidth();
        loadedH = window.getHeight();
        FSImGui::Shutdown();
        LOG_INFO("ImGui terminated");
        // OpenAL.
        phoneAmbience.remove();
        audio.remove();
        FSOAL::deinitialize();
        LOG_INFO("OpenAL terminated.");
        // Materials & objects
        for (size_t i = 0; i < entities.size(); i++)
            entities[i].remove();
        entities.clear();
        for (size_t i = 0; i < materialReg.size(); i++)
            materialReg[i].remove();
        materialReg.clear();
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
    txt["dev"]["shdrParamMaterialGenPreview"] = shdrParamMaterialGenPreview;

    txt["layout"]["atmosphere"] = atmosphereOpen;
    txt["layout"]["console"] = consoleOpen;
    txt["layout"]["entityEditor"] = entityEditorOpen;
    txt["layout"]["infoPanel"] = infoPanelOpen;
    txt["layout"]["lighting"] = lightingOpen;
    txt["layout"]["metrics"] = threadsOpen;
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
    if(!txt["window"].is_null()) {
        loadedW = txt["window"]["width"];
        loadedH = txt["window"]["height"];
    }
    if(!txt["dev"].is_null()) {
        if(!txt["dev"]["backupLogs"].is_null())                     backupLogs = txt["dev"]["backupLogs"];
        if(!txt["dev"]["consoleDevMode"].is_null())                 consoleDevMode = txt["dev"]["consoleDevMode"];
        if(!txt["dev"]["shdrParamMaterialGenPreview"].is_null())    shdrParamMaterialGenPreview = txt["dev"]["shdrParamMaterialGenPreview"];
    }
    if(!txt["layout"].is_null()) {
        txt = txt.at("layout");
        if(!txt["atmosphere"].is_null())         atmosphereOpen = txt["atmosphere"];
        if(!txt["console"].is_null())            consoleOpen = txt["console"];
        if(!txt["entityEditor"].is_null())       entityEditorOpen = txt["entityEditor"];
        if(!txt["infoPanel"].is_null())          infoPanelOpen = txt["infoPanel"];
        if(!txt["lighting"].is_null())           lightingOpen = txt["lighting"];
        if(!txt["metrics"].is_null())            threadsOpen = txt["metrics"];
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

    float velocity = 2.5f * tDeltaTime * speed_mult;
    bool isPressingMoveKey = false;

    if (glfwGetKey(tPtr, GLFW_KEY_M) == GLFW_PRESS)
        tWin->toggleVSync();

    if (glfwGetKey(tPtr, GLFW_KEY_W) == GLFW_PRESS) {
        camera.pos += camera.Forward * velocity;
        isPressingMoveKey = true;
    }
    if (glfwGetKey(tPtr, GLFW_KEY_S) == GLFW_PRESS) {
        camera.pos -= camera.Forward * velocity;
        isPressingMoveKey = true;
    }
    if (glfwGetKey(tPtr, GLFW_KEY_A) == GLFW_PRESS) {
        camera.pos -= camera.Right * velocity;
        isPressingMoveKey = true;
    }
    if (glfwGetKey(tPtr, GLFW_KEY_D) == GLFW_PRESS) {
        camera.pos += camera.Right * velocity;
        isPressingMoveKey = true;
    }

    if ((glfwGetKey(tPtr, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) && isPressingMoveKey)
        speed_mult += 0.005f;
    else speed_mult = 1.f;

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