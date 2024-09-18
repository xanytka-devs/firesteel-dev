#include "../include/window.hpp"
#include "../include/camera.hpp"
#include "../include/transform.hpp"
#include "../include/fbo.hpp"
#include "../include/2d/text.hpp"
#include "../include/particles.hpp"
#include "../include/light.hpp"
#include "../include/cubemap.hpp"
using namespace LearningOpenGL;
#include <stdio.h>
#include <time.h>
#include <../include/utils/imgui/markdown.hpp>
#include <../include/audio/base.hpp>
#include <../include/audio/wav.hpp>

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
bool fboSRGB = true;
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
bool newsViewOpen = true;
GLuint texID = 0;

int main() {
#pragma region Startup
    LOG_STATE("STARTUP");
    // Create window.
    Window win(800,600);
    if(!win.initialize("FiresteelDev App 0.2.0.1")) return 1;
    win.setClearColor(glm::vec3(0.185f, 0.15f, 0.1f));
    // GLAD (OpenGL) init.
    if(gladLoadGL(glfwGetProcAddress) == 0) {
        LOG_ERRR("Failed to initialize OpenGL context");
        return -1;
    }
    // Getting ready text renderer.
    Shader textShader("res/text.vs", "res/text.fs");
    TextRenderer::initialize();
    t.loadFont("res/fonts/vgasysr.ttf", 16);
    //OpenGL setup.
    displayLoadingMsg("Initializing OpenGL", &textShader, &win);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    fboSRGB ? glEnable(GL_FRAMEBUFFER_SRGB) : glDisable(GL_FRAMEBUFFER_SRGB);
    glfwSetCursorPosCallback(win.ptr(), mouseCallback);
    glfwSetScrollCallback(win.ptr(), scrollCallback);
    // OpenGL info.
    LOG_INFO("OpenGL context created:");
    LOG_INFO("	Vendor: ", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    LOG_INFO("	Renderer: ", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    LOG_INFO("	Version: ", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    // Shaders.
    displayLoadingMsg("Compiling shaders", &textShader, &win);
    Shader modelShader("res/model.vs", "res/model.fs");
    Shader fboShader("res/fbo.vs", "res/fbo.fs");
    Shader billboardShader("res/billboard.vs", "res/billboard.fs");
    Shader particlesShader("res/particles.vs", "res/particles.fs");
    Shader skyShader("res/skybox.vs", "res/skybox.fs");

    ParticleSystem pS = ParticleSystem(glm::vec3(0, 0, 0), 500, "res/particle.png");
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
        // Attenuation.
        1.f, // Constant.
        0.09f, // Linear.
        0.032f // Quadratic.
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
       1.f, // Constant.
       0.09f, // Linear.
       0.032f // Quadratic.
    };
    {
        modelShader.enable();
        modelShader.setFloat("material.emissionFactor", 1);
        modelShader.setFloat("material.shininess", 64);
        modelShader.setFloat("material.skyboxRefraction", 1.00f / 1.52f);
        modelShader.setFloat("material.skyboxRefractionStrength", 0.025f);

        modelShader.setVec3("dirLight.direction", dLight.direction);
        modelShader.setVec3("dirLight.ambient", dLight.ambient);
        modelShader.setVec3("dirLight.diffuse", dLight.diffuse);
        modelShader.setVec3("dirLight.specular", dLight.specular);

        modelShader.setInt("numPointLights", 1);
        modelShader.setVec3("pointLights[0].position", pLight.position);
        modelShader.setVec3("pointLights[0].ambient", pLight.ambient);
        modelShader.setVec3("pointLights[0].diffuse", pLight.diffuse);
        modelShader.setVec3("pointLights[0].specular", pLight.specular);
        modelShader.setFloat("pointLights[0].constant", pLight.constant);
        modelShader.setFloat("pointLights[0].linear", pLight.linear);
        modelShader.setFloat("pointLights[0].quadratic", pLight.quadratic);

        modelShader.setInt("numSpotLights", 1);
        modelShader.setVec3("spotLights[0].position", sLight.position);
        modelShader.setVec3("spotLights[0].direction", sLight.direction);
        modelShader.setVec3("spotLights[0].ambient", sLight.ambient);
        modelShader.setVec3("spotLights[0].diffuse", sLight.diffuse);
        modelShader.setVec3("spotLights[0].specular", sLight.specular);
        modelShader.setFloat("spotLights[0].cutOff", sLight.cutOff);
        modelShader.setFloat("spotLights[0].outerCutOff", sLight.outerCutOff);
        modelShader.setFloat("spotLights[0].constant", sLight.constant);
        modelShader.setFloat("spotLights[0].linear", sLight.linear);
        modelShader.setFloat("spotLights[0].quadratic", sLight.quadratic);
    }
    {
#define FOG_EQUATION_LINEAR 0
#define FOG_EQUATION_EXP 1
#define FOG_EQUATION_EXP2 2
        modelShader.setVec3("fogParams.vFogColor", glm::vec3(0.2f));
        modelShader.setFloat("fogParams.fStart", 10.0f);
        modelShader.setFloat("fogParams.fEnd", 20.0f);
        modelShader.setFloat("fogParams.fDensity", 0.04f);
        modelShader.setInt("fogParams.iEquation", FOG_EQUATION_EXP);
    }

    // Load models.
    //displayLoadingMsg("Loading city", &textShader, &win);
    //Transform city("res/city/scene.gltf");
    displayLoadingMsg("Loading backpack", &textShader, &win);
    Transform backpack("res/backpack/backpack.obj", fboSRGB);
    displayLoadingMsg("Loading quad", &textShader, &win);
    Transform quad("res/primitives/quad.obj", fboSRGB);
    displayLoadingMsg("Loading box", &textShader, &win);
    Transform box("res/box/box.obj", fboSRGB);
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
    Framebuffer fbo(win.getSize());
    fbo.quad();
    if(!fbo.isComplete()) LOG_ERRR("FBO isn't complete");
    // ImGUI setup.
    LOG_INFO("Initializing ImGui");
    displayLoadingMsg("Initializing ImGui", &textShader, &win);
    FSImGui::Initialize(&win);
    FSImGui::LoadFont("res/fonts/Ubuntu-Regular.ttf", 14);
    FSImGui::MD::LoadFonts("res/fonts/Ubuntu-Regular.ttf", "res/fonts/Ubuntu-Bold.ttf", 14, 15.4);
    LOG_INFO("ImGui ready");
    std::string newsTxtLoaded = StrFromFile("News.md");
    //OpenAL.
    char const* device_name = nullptr;
    device_name = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
    ALCdevice* openALDevice = alcOpenDevice(device_name);
    if (!openALDevice) {
        LOG_CRIT("OpenAL default device couldn't be open (How?).")
        return 0;
    }

    ALCcontext* openALContext = alcCreateContext(openALDevice, (ALCint*)nullptr);
    ALCboolean contextMadeCurrent = false;
    alcMakeContextCurrent(openALContext);

    std::uint8_t channels;
    std::int32_t sampleRate;
    std::uint8_t bitsPerSample;
    std::vector<char> soundData;
    FSOAL::load_wav("res/audio/coin.wav", channels, sampleRate, bitsPerSample, soundData);

    alGetError(); // clear error code 
    ALuint buffer;
    alGenBuffers(1, &buffer);
    if (alGetError() != AL_NO_ERROR) {
        std::cout << "ERRRR\n";
        return 1;
    }

    ALenum format = 0;
    if (channels == 1 && bitsPerSample == 8)
        format = AL_FORMAT_MONO8;
    else if (channels == 1 && bitsPerSample == 16)
        format = AL_FORMAT_MONO16;
    else if (channels == 2 && bitsPerSample == 8)
        format = AL_FORMAT_STEREO8;
    else if (channels == 2 && bitsPerSample == 16)
        format = AL_FORMAT_STEREO16;
    else {
        std::cerr << "ERROR: unrecognised format: " << std::to_string(channels) << " channels, " << std::to_string(bitsPerSample) << " bps"
            << std::endl;
    }

    alGetError(); // clear error code 
    alBufferData(buffer, format, soundData.data(), soundData.size(), sampleRate);
    //soundData.clear(); // erase the sound in RAM
    if (alGetError() != AL_NO_ERROR) {
        std::cout << "ERRRR\n";
        return 1;
    }

    ALuint source;
    alGenSources(1, &source);
    alSourcef(source, AL_PITCH, 1.f);
    alSource3f(source, AL_POSITION, 0, 0, 0);
    alSource3f(source, AL_VELOCITY, 0, 0, 0);
    alSourcei(source, AL_LOOPING, AL_FALSE);
    alSourcei(source, AL_BUFFER, buffer);
    ALfloat maxGain = 0.01f;
    alSourcef(source, AL_GAIN, maxGain);
    alSourcePlay(source);

#pragma endregion

    LOG_STATE("UPDATE LOOP");
    // Rendering.
    while(win.isOpen()) {
        win.pollEvents();
        //Per-frame time logic.
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        frame_count++;
        if(currentFrame - last_frame_fps >= 1.0) {
            fps = frame_count;
            frame_count = 0;
            last_frame_fps = currentFrame;
        }
        // Input processing.
        processInput(&win, win.ptr());
        if(glfwGetMouseButton(win.ptr(), 1) != GLFW_PRESS) win.setCursorMode(Window::CUR_NORMAL);
        else win.setCursorMode(Window::CUR_DISABLED);
        fbo.bind();
        win.clearBuffers();
        wireframeEnabled ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        fboSRGB ? glEnable(GL_FRAMEBUFFER_SRGB) : glDisable(GL_FRAMEBUFFER_SRGB);
        // View/projection transformations.
        glm::mat4 projection = camera.getProjection(win.aspect(), 1);
        glm::mat4 view = camera.getView();
        glm::mat4 model = glm::mat4(1.0f);
        sLight.position = camera.pos;
        sLight.direction = camera.Forward;
        sky.bind();
        modelShader.enable();
        modelShader.setInt("lightingType", lightingMode);
        modelShader.setBool("sRGBLighting", fboSRGB);
        modelShader.setVec3("spotLights[0].position", sLight.position);
        modelShader.setVec3("spotLights[0].direction", sLight.direction);
        // Render backpack.
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.f, 0.f, -1.f));
            model = glm::rotate(model, float(glm::radians(0.f)), glm::vec3(1, 0, 0));
            model = glm::rotate(model, float(glm::radians(0.f)), glm::vec3(0, 1, 0));
            model = glm::rotate(model, float(glm::radians(0.f)), glm::vec3(0, 0, 1));
            model = glm::scale(model, glm::vec3(0.5f));
            modelShader.setInt("DrawMode", drawMode);
            modelShader.setInt("skybox", 11);
            modelShader.setMat4("projection", projection);
            modelShader.setMat4("view", view);
            modelShader.setMat4("model", model);
            modelShader.setVec3("viewPos", camera.pos);
            modelShader.setVec3("material.emissionColor", glm::vec3(0));
            //city.draw(modelShader);
            backpack.draw(&modelShader);
        }
        // Render box.
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-2.f, 0.f, 1.f));
            model = glm::rotate(model, glm::radians((float)glfwGetTime() * -10.0f), glm::vec3(1));
            model = glm::scale(model, glm::vec3(1));
            modelShader.enable();
            modelShader.setMat4("model", model);
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
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.f, 5.f, 0.f));
            glActiveTexture(0);
            glBindTexture(GL_TEXTURE_2D, billTex.id);
            billboardShader.enable();
            billboardShader.setInt("DrawMode", drawMode);
            billboardShader.setMat4("projection", projection);
            billboardShader.setMat4("view", view);
            billboardShader.setVec3("color", glm::vec3(1));
            billboardShader.setVec2("size", glm::vec2(1.f));
            billboardShader.setMat4("model", model);
            quad.draw(&billboardShader);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(0);
        }
        // Draw point light billboard.
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pLight.position);
            glDepthFunc(GL_ALWAYS);
            glActiveTexture(0);
            glBindTexture(GL_TEXTURE_2D, pointLightBillTex.id);
            billboardShader.setVec3("color", pLight.diffuse);
            billboardShader.setVec2("size", glm::vec2(0.25f));
            billboardShader.setMat4("model", model);
            quad.draw(&billboardShader);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(0);
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
            fbo.unbind();
            win.clearBuffers();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            fboShader.enable();
            fboShader.setBool("sRGBLighting", fboSRGB);
            fboShader.setInt("AAMethod", 1);
            fboShader.setFloat("exposure", 1);
            fboShader.setVec2("screenSize", win.getSize());
            fboShader.setVec3("chromaticOffset", glm::vec3(10));
            fbo.drawQuad(&fboShader);
        }
        // Draw UI.
        if(drawNativeUI) {
            t.draw(&textShader, std::to_string(fps), win.getSize(), glm::vec2(8.0f, 568.0f), glm::vec2(1.5f), glm::vec3(0));
            t.draw(&textShader, std::to_string(fps), win.getSize(), glm::vec2(10.0f, 570.0f), glm::vec2(1.5f), glm::vec3(1, 0, 0));

            t.draw(&textShader, currentDateTime(), win.getSize(), glm::vec2(8.0f, 553.0f), glm::vec2(1.f), glm::vec3(0));
            t.draw(&textShader, currentDateTime(), win.getSize(), glm::vec2(10.0f, 555.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
            
            t.draw(&textShader, std::string("[V] VSync: ") + (win.getVSync() ? "ON" : "OFF"), win.getSize(), glm::vec2(8.0f, 538.0f), glm::vec2(1.f), glm::vec3(0));
            t.draw(&textShader, std::string("[V] VSync: ") + (win.getVSync() ? "ON" : "OFF"), win.getSize(), glm::vec2(10.0f, 540.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
            
            t.draw(&textShader, std::string("[G] sRGB Space: ") + (fboSRGB ? "ON" : "OFF"), win.getSize(), glm::vec2(8.0f, 523.0f), glm::vec2(1.f), glm::vec3(0));
            t.draw(&textShader, std::string("[G] sRGB Space: ") + (fboSRGB ? "ON" : "OFF"), win.getSize(), glm::vec2(10.0f, 525.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));

            t.draw(&textShader, "[Wheel] FOV: " + std::to_string((int)camera.fov), win.getSize(), glm::vec2(8.0f, 508.0f), glm::vec2(1.f), glm::vec3(0));
            t.draw(&textShader, "[Wheel] FOV: " + std::to_string((int)camera.fov), win.getSize(), glm::vec2(10.0f, 510.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));

            if (drawSelectMod == 0) {
                t.draw(&textShader, "[1-0] Shading: " + getDrawModName(), win.getSize(), glm::vec2(8.0f, 493.0f), glm::vec2(1.f), glm::vec3(0));
                t.draw(&textShader, "[1-0] Shading: " + getDrawModName(), win.getSize(), glm::vec2(10.0f, 495.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
            } else {
                t.draw(&textShader, "[1-4] Lighting: " + getLightingTypeName(), win.getSize(), glm::vec2(8.0f, 493.0f), glm::vec2(1.f), glm::vec3(0));
                t.draw(&textShader, "[1-4] Lighting: " + getLightingTypeName(), win.getSize(), glm::vec2(10.0f, 495.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
            }
        }
        // Draw ImGui.
        if(drawImGUI) {
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
            if(ImGui::BeginMenuBar()) {
                if(ImGui::BeginMenu(u8"Файл")) {
                    if (ImGui::MenuItem(u8"Сохранить")) {
                        FileDialog fd;
                        fd.filter = "All\0*.*\0HTML Document (*.html)\0*.HTML\0";
                        fd.filter_id = 2;
                        std::string res = fd.save();
                        if (res != "") {
                            if(!StrEndsWith(StrToLower(res).c_str(), ".html")) res.append(".html");
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
                if (ImGui::BeginMenu(u8"Окно")) {
                    ImGui::Checkbox(u8"Просмотр текстур", &texViewOpen);
                    ImGui::Checkbox(u8"Новости", &newsViewOpen);
                    ImGui::Separator();
                    if (ImGui::MenuItem(u8"Сбросить")) {
                        texViewOpen = false;
                        newsViewOpen = true;
                    }
                    ImGui::EndMenu();
                }
                if(ImGui::BeginMenu(u8"Тестирование")) {
                    if(ImGui::MenuItem(u8"Переключить Нативный UI")) { drawNativeUI = !drawNativeUI; }
                    if(ImGui::MenuItem(u8"Выключить ImGui")) {
                        drawImGUI = false;
                        LOG_INFO("ImGui rendering sequence disabled.\n    To enable it back... IDK, restart, maybe?");
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();
            if(newsViewOpen) {
                ImGui::Begin("News", &newsViewOpen);
                FSImGui::MD::Text(newsTxtLoaded);
                ImGui::End();
            }
            if(texViewOpen) {
                ImGui::Begin("Texture Viewer", &texViewOpen);
                float wW = ImGui::GetWindowWidth();
                int column = 3;
                if(wW > 400.f) column = 4;
                if(wW > 525.f) column = 5;
                if(wW > 650.f) column = 6;
                if(wW > 775.f) column = 7;
                if(wW > 900.f) column = 8;
                if(wW > 1025.f) column = 9;
                if(wW > 1150.f) column = 10;
                ImGui::BeginTable("Textures", column);
                glActiveTexture(GL_TEXTURE10);
                for (GLuint i = 0; i < 255; i++) {
                    if(glIsTexture(i)==GL_FALSE) continue;
                    glBindTexture(GL_TEXTURE_2D, i);
                    ImGui::BeginGroup();
                    ImGui::Image((void*)i, ImVec2(100, 100));
                    if(ImGui::Button(std::to_string(i).c_str())) {
                        texID = i;
                        fullTexViewOpen = true;
                    }
                    ImGui::EndGroup();
                    ImGui::TableNextColumn();
                }

                ImGui::EndTable();
                ImGui::End();
            }
            if (fullTexViewOpen) {
                ImGui::Begin("Full Texture Viewer", &fullTexViewOpen);
                glActiveTexture(GL_TEXTURE10);
                glBindTexture(GL_TEXTURE_2D, texID);
                ImGui::Image((void*)texID, ImVec2(ImGui::GetWindowWidth() * 0.9f, ImGui::GetWindowHeight() * 0.9f));
                ImGui::End();
            }

            // Rendering    
            FSImGui::Render(&win);
        }
        win.swapBuffers();
    }
    LOG_STATE("SHUTDOWN");
    FSImGui::Shutdown();
    LOG_INFO("ImGui terminated");
    // OpenAL.
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);

    alcCloseDevice(openALDevice);
    alcDestroyContext(openALContext);
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

    if (glfwGetKey(tPtr, GLFW_KEY_G) == GLFW_PRESS) { fboSRGB = !fboSRGB; }
    if (glfwGetKey(tPtr, GLFW_KEY_F) == GLFW_PRESS) { wireframeEnabled = !wireframeEnabled; }
}
float MouseSensitivity = 0.1f;
void mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    if(glfwGetMouseButton(window,1)!=GLFW_PRESS) return;
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
    t_Window->clearBuffers();
    t.draw(t_Shader, "App is loading", t_Window->getSize(),
        glm::vec2(t_Window->getWidth() / 3, t_Window->getHeight() / 2), glm::vec2(1.5f), glm::vec3(0.75f, 0.5f, 0.f));
    t.draw(t_Shader, t_Msg, t_Window->getSize(),
        glm::vec2(10, 15), glm::vec2(1.f), glm::vec3(1));
    t_Window->swapBuffers();
}