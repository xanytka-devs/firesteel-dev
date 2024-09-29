#include "../include/2d/text.hpp"
#include "../include/camera.hpp"
#include "../include/cubemap.hpp"
#include "../include/entity.hpp"
#include "../include/fbo.hpp"
#include "../include/light.hpp"
#include "../include/particles.hpp"
#include "../include/window.hpp"
using namespace LearningOpenGL;

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

#pragma region Defines
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0, 0, -90));
float lastX = 800 / 2.0f;
float lastY = 600 / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
int fps = 0;
double last_frame_fps = 0.f;
double last_frame = 0.f;
int frame_count = 0;

int drawSelectMod = 0; // 0=Textures, 1=Lightning
int drawMode = 0;
int lightingMode = 3;
bool wireframeEnabled = false;
bool drawNativeUI = true;
bool drawImGUI = true;

void processInput(Window* tWin, GLFWwindow* tPtr);
void mouseCallback(GLFWwindow* window, double xposIn, double yposIn);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
/// Get current date/time, format is YYYY-MM-DD HH:mm:ss
static const std::string currentDateTime();
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
bool metricsOpen = true;
bool lightingOpen = false;
bool atmosphereOpen = false;
bool displayGizmos = true;
GLuint texID = 0;
bool drawSkybox = true;

glm::mat4 curModel;

// Timers.
bool threadRuntime[3] = { false,false, false };
int _gpuThreadTime = 0;
static void _gpuTTCall() {
    _gpuThreadTime+=1;
}
static void _gpuThreadTimer() {
    threadRuntime[0] = true;
    _gpuThreadTime = 0;
    while (threadRuntime[0]) {
        _gpuTTCall();
    }
}
int _drawThreadTime = 0;
static void _drawTTCall() {
    _drawThreadTime += 1;
}
static void _drawThreadTimer() {
    threadRuntime[1] = true;
    _drawThreadTime = 0;
    while (threadRuntime[1]) {
        _drawTTCall();
    }
}
int _imguiThreadTime = 0;
static void _imguiTTCall() {
    _imguiThreadTime += 1;
}
static void _imguiThreadTimer() {
    threadRuntime[2] = true;
    _imguiThreadTime = 0;
    while (threadRuntime[2]) {
        _imguiTTCall();
    }
}
#pragma endregion

DirectionalLight dLight{
        glm::vec3(-0.2f, -1.0f, -0.3f), // Direction.
        // Lighting params.
        glm::vec3(0.f), // Ambient.
        glm::vec3(0.f), // Diffuse.
        glm::vec3(10.f) // Specular.
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
   glm::vec3(0.f), // Ambient.
   glm::vec3(1.0f, 0.5f, 0.31f), // Diffuse.
   glm::vec3(1.f), // Specular.
   // Attenuation.
   glm::cos(glm::radians(12.5f)), // Cutoff.
   glm::cos(glm::radians(17.5f)), // Outer cutoff.
};
static void updateLightInShader(Shader* tShader) {
    tShader->enable();
    tShader->setFloat("material.emissionFactor", 1);
    tShader->setFloat("material.shininess", 64);
    tShader->setFloat("material.skyboxRefraction", 1.00f / 1.52f);
    tShader->setFloat("material.skyboxRefractionStrength", 0.025f);

    tShader->setVec3("dirLight.direction", dLight.direction);
    tShader->setVec3("dirLight.ambient", dLight.ambient);
    tShader->setVec3("dirLight.diffuse", dLight.diffuse);
    tShader->setVec3("dirLight.specular", dLight.specular);

    tShader->setInt("numPointLights", 1);
    tShader->setVec3("pointLights[0].position", pLight.position);
    tShader->setVec3("pointLights[0].ambient", pLight.ambient);
    tShader->setVec3("pointLights[0].diffuse", pLight.diffuse);
    tShader->setVec3("pointLights[0].specular", pLight.specular);

    tShader->setInt("numSpotLights", 1);
    tShader->setVec3("spotLights[0].position", sLight.position);
    tShader->setVec3("spotLights[0].direction", sLight.direction);
    tShader->setVec3("spotLights[0].ambient", sLight.ambient);
    tShader->setVec3("spotLights[0].diffuse", sLight.diffuse);
    tShader->setVec3("spotLights[0].specular", sLight.specular);
    tShader->setFloat("spotLights[0].cutOff", sLight.cutOff);
    tShader->setFloat("spotLights[0].outerCutOff", sLight.outerCutOff);
}
#define FOG_EQUATION_LINEAR 0
#define FOG_EQUATION_EXP 1
#define FOG_EQUATION_EXP2 2
struct Fog {
    float Start = 20.f;
    float End = 25.f;
    float Density = 0.04f;
    glm::vec3 Color = glm::vec3(0.2f);
    int equation = FOG_EQUATION_EXP;
};
Fog fog{};
static void updateFogInShader(Shader* tShader, Fog* tFog) {
    tShader->setVec3("fogParams.vFogColor", tFog->Color);
    tShader->setFloat("fogParams.fStart", tFog->Start);
    tShader->setFloat("fogParams.fEnd", tFog->End);
    tShader->setFloat("fogParams.fDensity", tFog->Density);
    tShader->setInt("fogParams.iEquation", tFog->equation);
}

void APIENTRY glDebugOutput(GLenum source,
    GLenum type,
    unsigned int id,
    GLenum severity,
    GLsizei length,
    const char* message,
    const void* userParam)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
    case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}

#define BOUND_GL_VERSION_MAJOR 4
#define BOUND_GL_VERSION_MINOR 3

int main() {
#pragma region Startup
    LOG(      "Firesteel 0.2.0.3");
    LOG_C(    "[-   Dev branch  -]\n", CMD_F_PURPLE);
    LOG_STATE("STARTUP");
    // Create window.
    Window win(800,600);
    if(!win.initialize("FiresteelDev App 0.2.0.3",false, BOUND_GL_VERSION_MAJOR, BOUND_GL_VERSION_MINOR))
        return 1;
    win.setClearColor(glm::vec3(0.185f, 0.15f, 0.1f));

    printf("\n");
    //Check for Vulkan.
    bool isVulkan = (glfwVulkanSupported() == 1);
    LOG_INFO(isVulkan ? "Vulkan is supported on current machine."
        : "Vulkan isn't supported on current machine.");
    if (isVulkan) {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        LOG_INFO("Vulkan extensions: ");
        for (size_t i = 0; i < extensions.size(); i++) {
            LOG_C(std::string("  ") + extensions[i]);
        }
        printf("\n");
    }

    // GLAD (OpenGL) init.
    bool isOGL = (gladLoadGL(glfwGetProcAddress) != 0);

    if(!isOGL) {
        LOG_ERRR("OpenGL isn't supported on current machine.");
        return -1;
    } else LOG_INFO("OpenGL is supported on current machine.");
    // OpenGL info.
    LOG_INFO("OpenGL context created:");
    LOG_INFO(std::string("	Vendor: ") + reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    LOG_INFO(std::string("	Renderer: ") + reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    LOG_INFO(std::string("	Version: ") + reinterpret_cast<const char*>(glGetString(GL_VERSION)) + "\n");

    bool atLeastOneUnsup = false;
    if(!glfwExtensionSupported("GL_ARB_debug_output")) {
        LOG_ERRR("General debug output isn't supported.");
        atLeastOneUnsup = true;
    }
    if(!glfwExtensionSupported("GL_AMD_debug_output")) {
        LOG_ERRR("AMD debug output isn't supported.");
        atLeastOneUnsup = true;
    }
    if(!glfwExtensionSupported("GL_ARB_direct_state_access")) {
        LOG_ERRR("Official DSA isn't supported.");
        atLeastOneUnsup = true;
    }
    if (!glfwExtensionSupported("GL_EXT_direct_state_access")) {
        LOG_ERRR("Unofficial DSA isn't supported.");
        atLeastOneUnsup = true;
    }
    if(atLeastOneUnsup) printf("\n");
    int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        // initialize debug output 
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        if (BOUND_GL_VERSION_MAJOR >= 4) {
            glDebugMessageCallback(glDebugOutput, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        }
    }

    // Getting ready text renderer.
    Shader textShader("res/text.vs", "res/text.fs");
    TextRenderer::initialize();
    t.loadFont("res/fonts/vgasysr.ttf", 16);
    // OpenGL setup.
    displayLoadingMsg("Initializing OpenGL", &textShader, &win);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glfwSetCursorPosCallback(win.ptr(), mouseCallback);
    glfwSetScrollCallback(win.ptr(), scrollCallback);
    // OpenAL setup.
    displayLoadingMsg("Initializing OpenAL", &textShader, &win);
    FSOAL::initialize();
    FSOAL::Source audio;
    FSOAL::AudioLayer alBG{ 1.0f,1.0f };
    audio.initialize();
    audio.load("res/audio/elevator-music.mp3");
    audio.setGain(0.2f);
    audio.setLooping(true);
    audio.play();
    // OpenAL info.
    LOG_INFO("OpenAL context created:");
    LOG_INFO(std::string("	Device: ") + alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER) + "\n");
    // Shaders.
    displayLoadingMsg("Compiling shaders", &textShader, &win);
    Shader modelShader("res/model.vs", "res/model.fs");
    Shader fboShader("res/fbo.vs", "res/fbo.fs");
    Shader billboardShader("res/billboard.vs", "res/billboard.fs");
    Shader particlesShader("res/particles.vs", "res/particles.fs");
    Shader skyShader("res/skybox.vs", "res/skybox.fs");
    ParticleSystem pS = ParticleSystem(glm::vec3(0, 0, 0), 500, "res/particle.png");

    // Load models.
    //displayLoadingMsg("Loading city", &textShader, &win);
    //Transform city("res/city/scene.gltf");
    displayLoadingMsg("Loading backpack", &textShader, &win);
    Entity backpack("res/backpack/backpack.obj", glm::vec3(0.f, 0.f, -1.f), glm::vec3(0),glm::vec3(0.5));
    displayLoadingMsg("Loading quad", &textShader, &win);
    Entity quad("res/primitives/quad.obj");
    displayLoadingMsg("Loading box", &textShader, &win);
    Entity box("res/box/box.obj", glm::vec3(-2.f, 0.f, 1.f));
    displayLoadingMsg("Loading phone booth", &textShader, &win);
    Entity phoneBooth("res/phone-booth/X.obj", glm::vec3(10.f, 0.f, 10.f), glm::vec3(0), glm::vec3(0.5f));
    Texture billTex;
    billTex.id = TextureFromFile("res/yeah.png");
    billTex.type = "texture_diffuse";
    billTex.path = "res/yeah.png";
    Texture pointLightBillTex;
    pointLightBillTex.id = TextureFromFile("res/billboards/point_light.png");
    pointLightBillTex.type = "texture_diffuse";
    pointLightBillTex.path = "res/billboards/point_light.png";
    // Cubemap.
    displayLoadingMsg("Creating cubemap", &textShader, &win);
    Cubemap sky;
    sky.load_m("res/FortPoint", "posz.jpg", "negz.jpg", "posy.jpg", "negy.jpg", "posx.jpg", "negx.jpg");
    sky.initialize(100);
    // Framebuffer.
    displayLoadingMsg("Creating FBO", &textShader, &win);
    Framebuffer ppFBO(win.getSize(), 2);
    Framebuffer imguiFBO(win.getSize());
    ppFBO.quad();
    if(!ppFBO.isComplete()) LOG_ERRR("FBO isn't complete");
    if(!imguiFBO.isComplete()) LOG_ERRR("FBO isn't complete");
    camera.farPlane = 1000;
    // ImGUI setup.
    LOG_INFO("Initializing ImGui");
    displayLoadingMsg("Initializing ImGui", &textShader, &win);
    FSImGui::Initialize(&win);
    FSImGui::LoadFont("res/fonts/Ubuntu-Regular.ttf", 14, true);
    FSImGui::MD::LoadFonts("res/fonts/Ubuntu-Regular.ttf", "res/fonts/Ubuntu-Bold.ttf", 14, 15.4f);
    LOG_INFO("ImGui ready");
    std::string newsTxtLoaded = StrFromFile("News.md");
    //OpenAL.
    FSOAL::Source phoneAmbience;
    FSOAL::AudioLayer alSFX{ 1.f,1.0f };
    phoneAmbience.initialize();
    phoneAmbience.load("res/audio/tone.wav");
    phoneAmbience.setGain(0.1f);
    phoneAmbience.setLooping(true);
    phoneAmbience.setPostion(10.f, 0.f, 10.f);

    audio.remove();
    audio.initialize();
    audio.load("res/audio/harbour-port-ambience.wav");
    audio.play();
    phoneAmbience.play();

#pragma endregion

    LOG_STATE("UPDATE LOOP");
    // Rendering.
    while (win.isOpen()) {
        if (!drawImGUI)
            if (ppFBO.getSize() != win.getSize())
                ppFBO.scale(win.getSize());
        win.pollEvents();
        std::thread GPUThreadT(_gpuThreadTimer);
        //Per-frame time logic.
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        frame_count++;
        if (currentFrame - last_frame_fps >= 1.0) {
            fps = frame_count;
            frame_count = 0;
            last_frame_fps = currentFrame;
        }
        // Input processing.
        processInput(&win, win.ptr());
        audio.setPostion(camera.pos);
        if (glfwGetMouseButton(win.ptr(), 1) != GLFW_PRESS) win.setCursorMode(Window::CUR_NORMAL);
        else win.setCursorMode(Window::CUR_DISABLED);
        ppFBO.bind();
        updateLightInShader(&modelShader);
        updateFogInShader(&modelShader, &fog);
        win.clearBuffers();
        wireframeEnabled ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        // View/projection transformations.
        glm::mat4 projection = camera.getProjection(win.aspect(), 1);
        glm::mat4 view = camera.getView();
        glm::mat4 model = glm::mat4(1.0f);
        sLight.position = camera.pos;
        sLight.direction = camera.Forward;
        std::thread DrawThreadT(_drawThreadTimer);
        sky.bind();
        modelShader.enable();
        modelShader.setInt("lightingType", lightingMode);
        modelShader.setVec3("spotLights[0].position", sLight.position);
        modelShader.setVec3("spotLights[0].direction", sLight.direction);
        // Render backpack.
        {
            modelShader.setInt("DrawMode", drawMode);
            modelShader.setInt("skybox", 11);
            modelShader.setMat4("projection", projection);
            modelShader.setMat4("view", view);
            modelShader.setVec3("viewPos", camera.pos);
            modelShader.setVec3("material.emissionColor", glm::vec3(0));
            //city.draw(modelShader);
            backpack.draw(&modelShader);
        }
        // Render phone booth.
        {
            phoneBooth.draw(&modelShader);
        }
        // Render box.
        {
            box.transform.Rotation = glm::vec3((float)glfwGetTime() * -10.0f);
            modelShader.setVec3("material.emissionColor", glm::vec3(0.0f, 0.5f, 0.69f));
            box.draw(&modelShader);
        }
        // Particle system.
        {
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
        // Draw billboard.
        {
            quad.transform.Position = glm::vec3(0.f, 5.f, 0.f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, billTex.id);
            billboardShader.enable();
            billboardShader.setInt("DrawMode", drawMode);
            billboardShader.setMat4("projection", projection);
            billboardShader.setMat4("view", view);
            billboardShader.setVec3("color", glm::vec3(1));
            billboardShader.setVec2("size", glm::vec2(1.f));
            quad.draw(&billboardShader);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
        }
        // Draw point light billboard.
        if(displayGizmos) {
            quad.transform.Position = pLight.position;
            glDepthFunc(GL_ALWAYS);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pointLightBillTex.id);
            billboardShader.setVec3("color", pLight.diffuse);
            billboardShader.setVec2("size", glm::vec2(0.25f));
            billboardShader.setMat4("model", model);
            quad.draw(&billboardShader);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
            glDepthFunc(GL_LESS);
        }
        // Skybox.
        {
            skyShader.enable();
            skyShader.setInt("DrawMode", drawMode);
            skyShader.setMat4("projection", projection);
            skyShader.setMat4("view", view);
            sky.draw(&skyShader);
        }
        // Draw FBO.
        {
            ppFBO.unbind();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            fboShader.enable();
            fboShader.setInt("AAMethod", 1);
            fboShader.setFloat("exposure", 1);
            fboShader.setVec2("screenSize", win.getSize());
            fboShader.setVec3("chromaticOffset", glm::vec3(10));
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ppFBO.getID(0));
            fboShader.setInt("screenTexture", 1);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, ppFBO.getID(1));
            fboShader.setInt("bloomBlur", 2);
            if(drawImGUI) imguiFBO.bind();
            win.clearBuffers();
            ppFBO.drawQuad(&fboShader);
            if(drawImGUI) {
                imguiFBO.unbind();
                win.clearBuffers();
            }
        }
        threadRuntime[1] = false;
        DrawThreadT.join();
        // Draw UI.
        if(drawNativeUI) {
            t.draw(&textShader, std::to_string(fps), win.getSize(), glm::vec2(8.0f, 568.0f), glm::vec2(1.5f), glm::vec3(0));
            t.draw(&textShader, std::to_string(fps), win.getSize(), glm::vec2(10.0f, 570.0f), glm::vec2(1.5f), glm::vec3(1, 0, 0));

            t.draw(&textShader, currentDateTime(), win.getSize(), glm::vec2(8.0f, 553.0f), glm::vec2(1.f), glm::vec3(0));
            t.draw(&textShader, currentDateTime(), win.getSize(), glm::vec2(10.0f, 555.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));

            t.draw(&textShader, std::string("[V] VSync: ") + (win.getVSync() ? "ON" : "OFF"),
                win.getSize(), glm::vec2(8.0f, 538.0f), glm::vec2(1.f), glm::vec3(0));
            t.draw(&textShader, std::string("[V] VSync: ") + (win.getVSync() ? "ON" : "OFF"),
                win.getSize(), glm::vec2(10.0f, 540.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));

            t.draw(&textShader, "[Wheel] FOV: " + std::to_string((int)camera.fov),
                win.getSize(), glm::vec2(8.0f, 523.0f), glm::vec2(1.f), glm::vec3(0));
            t.draw(&textShader, "[Wheel] FOV: " + std::to_string((int)camera.fov),
                win.getSize(), glm::vec2(10.0f, 525.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));

            if (drawSelectMod == 0) {
                t.draw(&textShader, "[1-0] Shading: " + getDrawModName(),
                    win.getSize(), glm::vec2(8.0f, 508.0f), glm::vec2(1.f), glm::vec3(0));
                t.draw(&textShader, "[1-0] Shading: " + getDrawModName(),
                    win.getSize(), glm::vec2(10.0f, 510.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
            }
            else {
                t.draw(&textShader, "[1-4] Lighting: " + getLightingTypeName(),
                    win.getSize(), glm::vec2(8.0f, 508.0f), glm::vec2(1.f), glm::vec3(0));
                t.draw(&textShader, "[1-4] Lighting: " + getLightingTypeName(),
                    win.getSize(), glm::vec2(10.0f, 510.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
            }
        }
        // Draw ImGui.
        if (drawImGUI) {
            std::thread ImGuiThreadT(_imguiThreadTimer);
            // Start the Dear ImGui frame
            FSImGui::NewFrame();

            // Gui stuff.
            //Do fullscreen.
            ImGui::PopStyleVar(3);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGuiID dockspace_id = ImGui::GetID("Firesteel Dev Editor");
            ImGui::Begin("Firesteel Dev Editor", NULL, FSImGui::defaultDockspaceWindowFlags);
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), FSImGui::defaultDockspaceFlags);
            ImGui::PopStyleVar(3);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
            // Upper help menu.
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu(u8" Файл")) {
                    if (ImGui::MenuItem(u8"Сохранить")) {
                        FileDialog fd;
                        fd.filter = "All\0*.*\0HTML Document (*.html)\0*.HTML\0";
                        fd.filter_id = 2;
                        std::string res = fd.save();
                        if (res != "") {
                            if (!StrEndsWith(StrToLower(res).c_str(), ".html")) res.append(".html");
                            StrToFile(res, "\
<html><head><meta http-equiv=\"refresh\" content=\"0; url=https://www.youtube.com/watch?v=dQw4w9WgXcQ\" /></head></html>\
                            ");
                        }
                    }
                    if (ImGui::MenuItem(u8"Открыть")) {
                        FileDialog fd;
                        fd.filter = "All\0*.*\0HTML Document (*.html)\0*.HTML\0";
                        fd.filter_id = 2;
                        std::string res = fd.open();
                        if (res != "") {
                            openURL("https://www.youtube.com/watch?v=dQw4w9WgXcQ");
                        }
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem(u8"Закрыть (Esc)"))
                        win.close();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(u8" Окно")) {
                    ImGui::Checkbox(u8"Атмосфера", &atmosphereOpen);
                    ImGui::Checkbox(u8"Миксер аудио", &soundMixerOpen);
                    ImGui::Checkbox(u8"Новости", &newsViewOpen);
                    ImGui::Checkbox(u8"Просмотр сцены", &sceneViewOpen);
                    ImGui::Checkbox(u8"Свет", &lightingOpen);
                    ImGui::Separator();
                    if (ImGui::MenuItem(u8"Сбросить")) {
                        texViewOpen = false;
                        newsViewOpen = true;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(u8" Тестирование")) {
                    if (ImGui::MenuItem(u8"Переключить Нативный UI")) { drawNativeUI = !drawNativeUI; }
                    if (ImGui::MenuItem(u8"Выключить ImGui")) {
                        drawImGUI = false;
                        LOG_INFO("ImGui rendering sequence disabled.\n    To enable it back... IDK, restart, maybe?");
                    }
                    ImGui::Separator();
                    ImGui::Checkbox(u8"Просмотр текстур", &texViewOpen);
                    ImGui::Checkbox(u8"Статистика", &metricsOpen);
                    ImGui::Checkbox(u8"Отображать \"штуковины\"", &displayGizmos);
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();

            ImGui::PopStyleVar(3);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);

            if(newsViewOpen) {
                ImGui::Begin("News", &newsViewOpen);
                FSImGui::MD::Text(newsTxtLoaded);
                ImGui::End();
            }
            if(sceneViewOpen) {
                ImGui::Begin("Scene", &sceneViewOpen);
                ImVec2 winSceneSize = ImGui::GetWindowContentRegionMax();
                winSceneSize.y -= 20;
                ImGui::Image((void*)imguiFBO.getID(), winSceneSize, ImVec2(0, 1), ImVec2(1, 0));
                if (imguiFBO.getSize() != glm::vec2(winSceneSize.x, winSceneSize.y)) {
                    ppFBO.scale(static_cast<int>(winSceneSize.x), static_cast<int>(winSceneSize.y));
                    imguiFBO.scale(static_cast<int>(winSceneSize.x), static_cast<int>(winSceneSize.y));
                }

                ImGui::PopStyleVar(3);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);

                // Guizmos.
                if(displayGizmos) {
                    curModel = backpack.getMatrix();
                    ImGuizmo::SetOwnerWindowName("Scene");
                    ImGuizmo::BeginFrame();

                    // imguizmo
                    ImGuizmo::SetOrthographic(false);
                    ImGuizmo::AllowAxisFlip(false);
                    ImGuizmo::SetDrawlist();
                    // ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);
                    glm::vec4 vp = glm::vec4(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y,
                        ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
                    ImGuizmo::SetRect(vp.x, vp.y, vp.z, vp.w);
                    // ImGuizmo::GetStyle().CenterCircleSize = 3.0f;

                    static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
                    if (ImGui::IsKeyPressed(ImGuiKey_E)) currentGizmoOperation = ImGuizmo::TRANSLATE;
                    if (ImGui::IsKeyPressed(ImGuiKey_R)) currentGizmoOperation = ImGuizmo::ROTATE;
                    if (ImGui::IsKeyPressed(ImGuiKey_T)) currentGizmoOperation = ImGuizmo::SCALE;

                    // Define the mode (local or world space)
                    static ImGuizmo::MODE currentGizmoMode = ImGuizmo::LOCAL;
                    if (ImGui::IsKeyPressed(ImGuiKey_L)) currentGizmoMode = ImGuizmo::LOCAL;
                    if (ImGui::IsKeyPressed(ImGuiKey_K)) currentGizmoMode = ImGuizmo::WORLD;

                    // Manipulate the model matrix
                    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
                        currentGizmoOperation, currentGizmoMode, glm::value_ptr(curModel));

                    float transl[3] = { 0,0,0 };
                    float rotat[3] = { 0,0,0 };
                    float scalel[3] = { 1,1,1 };
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(curModel), transl, rotat, scalel);
                    backpack.transform.Position = float3ToVec3(transl);
                    backpack.transform.Rotation = float3ToVec3(rotat);
                    backpack.transform.Size = float3ToVec3(scalel);
                }
                ImGui::End();
            }
            if(atmosphereOpen) {
                ImGui::Begin("Atmosphere", &atmosphereOpen);
                if(ImGui::CollapsingHeader("Directional Light")) {
                    FSImGui::DragFloat3("Direction", &dLight.direction);
                    ImGui::Separator();
                    FSImGui::ColorEdit3("Ambient", &dLight.ambient);
                    FSImGui::ColorEdit3("Diffuse", &dLight.diffuse);
                    FSImGui::ColorEdit3("Specular", &dLight.specular);
                }
                if (ImGui::CollapsingHeader("Fog")) {
                    FSImGui::ColorEdit3("Color", &fog.Color);
                    ImGui::Separator();
                    ImGui::DragFloat("Start", &fog.Start);
                    ImGui::DragFloat("End", &fog.End);
                    ImGui::DragFloat("Density", &fog.Density);
                    ImGui::DragInt("Equation", &fog.equation, 1.f, 0, 2);
                }

                ImGui::End();
            }
            if(soundMixerOpen) {
                ImGui::Begin("Sound Mixer", &soundMixerOpen);
                {
                    ImGui::BeginGroup();
                    ImGui::Text("Background");
                    ImGui::BeginDisabled(bgMuted);
                    if(ImGui::VSliderFloat("##BackgroundSlider", ImVec2(40, 200), &alBG.gain, 0.0f, 1.0f, "")) {
                        audio.setGain(alBG);
                    }
                    std::string val = std::to_string(alBG.gain);
                    val = val.substr(0, 5);
                    ImGui::Text(val.c_str());
                    if(ImGui::Knob("##BackgroundKnob", &alBG.pitch, 0.01f, 5.0f, 0.0f, "%.3f",
                        ImGuiKnobVariant_Wiper, 0.0f, ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput)) {
                        audio.setPitch(alBG);
                    }
                    ImGui::EndDisabled();
                    if(ImGui::Button("M##Background")) {
                        if(!bgMuted) preMuteGain = alBG.gain;
                        bgMuted = !bgMuted;
                        if(bgMuted) alBG.gain = 0;
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
                    if(ImGui::VSliderFloat("##SFXSlider", ImVec2(40, 200), &alSFX.gain, 0.0f, 1.0f, "")) {
                        phoneAmbience.setGain(alSFX);
                    }
                    std::string val = std::to_string(alSFX.gain);
                    val = val.substr(0, 5);
                    ImGui::Text(val.c_str());
                    if(ImGui::Knob("##SFXKnob", &alSFX.pitch, 0.01f, 5.0f, 0.0f, "%.3f",
                        ImGuiKnobVariant_Wiper, 0.0f, ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput)) {
                        phoneAmbience.setPitch(alSFX);
                    }
                    ImGui::EndDisabled();
                    if(ImGui::Button("M##SFX")) {
                        if(!sfxMuted) preMuteGain = alSFX.gain;
                        sfxMuted = !sfxMuted;
                        if(sfxMuted) alSFX.gain = 0;
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
            if(lightingOpen){
                ImGui::Begin("Lighting", &lightingOpen);
                if(ImGui::CollapsingHeader("Point Light [1]")) {
                    FSImGui::DragFloat3("Position", &pLight.position);
                    ImGui::Separator();
                    FSImGui::ColorEdit3("Ambient", &pLight.ambient);
                    FSImGui::ColorEdit3("Diffuse", &pLight.diffuse);
                    FSImGui::ColorEdit3("Specular", &pLight.specular);
                }
                if(ImGui::CollapsingHeader("Spot Light [1]")) {
                    FSImGui::DragFloat3("Position", &sLight.position);
                    FSImGui::DragFloat3("Direction", &sLight.direction);
                    ImGui::Separator();
                    FSImGui::ColorEdit3("Ambient", &sLight.ambient);
                    FSImGui::ColorEdit3("Diffuse", &sLight.diffuse);
                    FSImGui::ColorEdit3("Specular", &sLight.specular);
                    ImGui::Separator();
                    ImGui::DragFloat("Cut off", &sLight.cutOff);
                    ImGui::DragFloat("Outer cut off", &sLight.outerCutOff);
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
                    ImGui::Image((void*)i, ImVec2(100, 100));
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
                ImGui::Image((void*)texID, ImVec2(ImGui::GetWindowWidth() * 0.9f, ImGui::GetWindowHeight() * 0.9f));
                glBindTexture(GL_TEXTURE_2D, 0);
                glActiveTexture(GL_TEXTURE0);
                ImGui::End();
            }

            threadRuntime[0] = false;
            GPUThreadT.join();
            threadRuntime[2] = false;
            ImGuiThreadT.join();
            if(metricsOpen) {
                ImGui::Begin("Threads", &metricsOpen);
                ImGui::Text(("FPS: " + std::to_string(fps)).c_str());
                ImGui::Text(("Delta time: " + std::to_string(deltaTime)).c_str());
                ImGui::Separator();
                ImGui::Text(("GPU Thread: " + std::to_string(_gpuThreadTime)).c_str());
                ImGui::Text(("Draw Thread: " + std::to_string(_drawThreadTime)).c_str());
                ImGui::Text(("ImGui Thread: " + std::to_string(_imguiThreadTime)).c_str());
                ImGui::End();
            }

            // Rendering
            FSImGui::Render(&win);
        }
        if (!drawImGUI) {
            threadRuntime[0] = false;
            GPUThreadT.join();
        }
        win.swapBuffers();
    }
    LOG_STATE("SHUTDOWN");
    FSImGui::Shutdown();
    LOG_INFO("ImGui terminated");
    // OpenAL.
    phoneAmbience.remove();
    audio.remove();

    FSOAL::deinitialize();
    // Quitting.
    win.terminate();
    LOG_INFO("Window terminated");
    LOG_STATE("QUIT");
	return 0;
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
void processInput(Window* tWin, GLFWwindow* tPtr) {
    if (glfwGetKey(tPtr, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        tWin->close();
    if (glfwGetMouseButton(tPtr, 1) != GLFW_PRESS) return;

    float velocity = 2.5f * deltaTime;
    if (glfwGetKey(tPtr, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        velocity *= speed_mult;
        speed_mult += 0.005f;
    }
    else speed_mult = 2.f;

    if (glfwGetKey(tPtr, GLFW_KEY_V) == GLFW_PRESS)
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

    if (glfwGetKey(tPtr, GLFW_KEY_F) == GLFW_PRESS) { wireframeEnabled = !wireframeEnabled; }
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
static const std::string currentDateTime() {
    struct tm newtime;
    __time64_t long_time;
    char timebuf[26];
    errno_t err;

    // Get time as 64-bit integer.
    _time64(&long_time);
    // Convert to local time.
    err = _localtime64_s(&newtime, &long_time);
    if(err) {
        LOG_WARN("Invalid argument to _localtime64_s.");
        return "invalid";
    }
    strftime(timebuf, sizeof(timebuf), "%d.%m.%Y %X", &newtime);
    return timebuf;
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