#include "../include/window.hpp"
#include "../include/camera.hpp"
#include "../include/transform.hpp"
#include "../include/fbo.hpp"
#include "../include/2d/text.hpp"
#include <stdio.h>
#include <time.h>
#include "../include/particles.hpp"
#include "../include/light.hpp"
#include "../include/cubemap.hpp"
using namespace LearningOpenGL;

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

void processInput(Window* tWin, GLFWwindow* tPtr);
void mouseCallback(GLFWwindow* window, double xposIn, double yposIn);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
/// Get current date/time, format is YYYY-MM-DD HH:mm:ss
static const std::string currentDateTime();
Text t;
static void displayLoadingMsg(std::string t_Msg, Shader* t_Shader, Window* t_Window);
static std::string getDrawModName();
static std::string getLightingTypeName();

int main() {
    // Create window.
    Window win(800,600);
    if(!win.initialize()) return 1;
    win.setClearColor(glm::vec3(0.185f, 0.15f, 0.1f));
    // GLAD (OpenGL) init.
    if(gladLoadGL(glfwGetProcAddress) == 0) {
        LOG_ERRR("Failed to initialize OpenGL context");
        return -1;
    }
    // Getting ready text renderer.
    Shader textShader("res/text.vs", "res/text.fs");
    TextRenderer::initialize();
    t.loadFont("res/vgasysr.ttf", 16);
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
    win.setCursorMode(Window::CUR_DISABLED);
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
    Framebuffer fbo(win.getSize(), true);
    fbo.quad();
    if(!fbo.isComplete()) LOG_ERRR("FBO isn't complete");
    camera.farPlane = 10000;
    // Rendering.
    while(win.isOpen()) {
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
            fbo.drawQuad(&fboShader);
        }
        // Draw UI.
        {
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
                t.draw(&textShader, "[1-9] Shading: " + getDrawModName(), win.getSize(), glm::vec2(8.0f, 493.0f), glm::vec2(1.f), glm::vec3(0));
                t.draw(&textShader, "[1-9] Shading: " + getDrawModName(), win.getSize(), glm::vec2(10.0f, 495.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
            } else {
                t.draw(&textShader, "[1-4] Lighting: " + getLightingTypeName(), win.getSize(), glm::vec2(8.0f, 493.0f), glm::vec2(1.f), glm::vec3(0));
                t.draw(&textShader, "[1-4] Lighting: " + getLightingTypeName(), win.getSize(), glm::vec2(10.0f, 495.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
            }
        }
        win.update();
    }
    // Quitting.
    win.terminate();
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
        if (glfwGetKey(tPtr, GLFW_KEY_9) == GLFW_PRESS) { drawMode = 1; }
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
    t_Window->update();
}