#include "../include/window.hpp"
#include "../include/camera.hpp"
#include "../include/transform.hpp"
#include "../include/fbo.hpp"
#include "../include/2d/text.hpp"
#include <stdio.h>
#include <time.h>
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
void processInput(Window* tWin, GLFWwindow* tPtr);
void mouseCallback(GLFWwindow* window, double xposIn, double yposIn);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
/// Get current date/time, format is YYYY-MM-DD HH:mm:ss
static const std::string currentDateTime();
Text t;
static void displayLoadingMsg(std::string t_Msg, Shader* t_Shader, Window* t_Window);

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
    // Load models.
    displayLoadingMsg("Loading backpack", &textShader, &win);
    Transform backpack("res/backpack/backpack.obj");
    // Framebuffer.
    displayLoadingMsg("Creating FBO", &textShader, &win);
    Framebuffer fbo(win.getSize());
    fbo.quad();
    if(!fbo.isComplete()) LOG_ERRR("FBO isn't complete");
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
        // View/projection transformations.
        glm::mat4 projection = camera.getProjection(win.aspect(), 1);
        glm::mat4 view = camera.getView();
        glm::mat4 model = glm::mat4(1.0f);
        // Render backpack.
        {
            model = glm::translate(model, glm::vec3(0.f, 0.f, 0.f));
            model = glm::scale(model, glm::vec3(0.25f));
            model = glm::rotate(model, float(glm::radians(0.f)), glm::vec3(1, 0, 0));
            model = glm::rotate(model, float(glm::radians(0.f)), glm::vec3(0, 1, 0));
            model = glm::rotate(model, float(glm::radians(0.f)), glm::vec3(0, 0, 1));
            modelShader.enable();
            modelShader.setMat4("projection", projection);
            modelShader.setMat4("view", view);
            modelShader.setMat4("model", model);
            backpack.draw(modelShader);
        }
        // Draw FBO.
        {
            fbo.unbind();
            win.clearBuffers();
            fboShader.enable();
            fboShader.setInt("AAMethod", 1);
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
            
            t.draw(&textShader, "[Wheel] FOV: " + std::to_string((int)camera.fov), win.getSize(), glm::vec2(8.0f, 523.0f), glm::vec2(1.f), glm::vec3(0));
            t.draw(&textShader, "[Wheel] FOV: " + std::to_string((int)camera.fov), win.getSize(), glm::vec2(10.0f, 525.0f), glm::vec2(1.f), glm::vec3(1, 0, 0));
        }
        win.update();
    }
    // Quitting.
    win.terminate();
	return 0;
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
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);

    return buf;
}
static void displayLoadingMsg(std::string t_Msg, Shader* t_Shader, Window* t_Window) {
    t_Window->clearBuffers();
    t.draw(t_Shader, "App is loading", t_Window->getSize(),
        glm::vec2(t_Window->getWidth() / 3, t_Window->getHeight() / 2), glm::vec2(1.5f), glm::vec3(0.75f, 0.5f, 0.f));
    t.draw(t_Shader, t_Msg, t_Window->getSize(),
        glm::vec2(10, 15), glm::vec2(1.f), glm::vec3(1));
    t_Window->update();
}