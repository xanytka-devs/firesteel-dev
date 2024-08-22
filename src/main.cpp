#include "../include/window.hpp"
#include "../include/camera.hpp"
#include "../include/transform.hpp"
#include "../include/fbo.hpp"
using namespace LearningOpenGL;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0, 0, -90));
float lastX = 800 / 2.0f;
float lastY = 600 / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
void processInput(Window* tWin, GLFWwindow* tPtr);
void mouseCallback(GLFWwindow* window, double xposIn, double yposIn);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

int main() {
    Window win;
    if(!win.initialize()) return 1;
    win.setClearColor(glm::vec3(0.185f, 0.15f, 0.1f));
    // GLAD (OpenGL) init.
    if(gladLoadGL(glfwGetProcAddress) == 0) {
        LOG_ERRR("Failed to initialize OpenGL context");
        return -1;
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glfwSetCursorPosCallback(win.ptr(), mouseCallback);
    glfwSetScrollCallback(win.ptr(), scrollCallback);
    //OpenGL info.
    LOG_INFO("OpenGL context created:");
    LOG_INFO("	Vendor: ", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    LOG_INFO("	Renderer: ", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    LOG_INFO("	Version: ", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    win.setCursorMode(Window::CUR_DISABLED);
    Shader modelShader("res/model.vs", "res/model.fs");
    Shader fboShader("res/fbo.vs", "res/fbo.fs");
    Transform backpack("res/backpack/backpack.obj");
    Framebuffer fbo(win.getSize());
    fbo.quad();
    if(!fbo.isComplete()) LOG_ERRR("FBO isn't complete");
    // Rendering.
    while(win.isOpen()) {
        // Delta time calculation.
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // Input processing.
        processInput(&win, win.ptr());
        fbo.bind();
        win.clearBuffers();
        // View/projection transformations.
        glm::mat4 projection = camera.getProjection(win.aspect(), 1);
        glm::mat4 view = camera.getView();
        // Render backpack.
        glm::mat4 model = glm::mat4(1.0f);
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
        fbo.unbind();
        win.clearBuffers();
        fboShader.enable();
        fboShader.setInt("AAMethod", 1);
        fboShader.setVec2("screenSize", win.getSize());
        fbo.drawQuad(&fboShader);
        win.update();
    }
    // Quitting.
    win.terminate();
	return 0;
}

float speed_mult = 2.f;
int fps = 0;
double last_frame_fps = 0.f;
double last_frame = 0.f;
int frame_count = 0;
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