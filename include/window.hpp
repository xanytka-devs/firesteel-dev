#ifndef WINDOW_H
#define WINDOW_H
#include "common.hpp"

namespace LearningOpenGL {
	class Window {
	public:
		Window(int t_width = 800, int t_height = 600, bool t_vsync = false) :
			mPtr(NULL), mWidth(t_width), mHeight(t_height), mVSync(t_vsync), mClearColor(glm::vec3(0)), mClosed(false) {}

		bool initialize(const char* t_title = "Firesteel App", bool t_fullscreen = false) {
            // Initialize and configure.
            LOG_INFO("Creating window \"", t_title, "\"");
            glfwInit();
            glfwWindowHint(GL_FRAMEBUFFER_SRGB, GL_TRUE);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwSetErrorCallback(errorCallback);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            // Window creation.
            GLFWmonitor* mon = NULL;
            if (t_fullscreen) mon = glfwGetPrimaryMonitor();
            mPtr = glfwCreateWindow(mWidth, mHeight, t_title, mon, NULL);
            if (mPtr == NULL) {
                LOG_ERRR("Failed to create GLFW window");
                glfwTerminate();
                return false;
            }
            glfwMakeContextCurrent(mPtr);
            // Setup vsync.
            if(mVSync) glfwSwapInterval(1);
            else glfwSwapInterval(0);
            // Setup parametrs.
            glfwInitHint(GLFW_JOYSTICK_HAT_BUTTONS, GLFW_TRUE);
            // Setup callbacks.
            glfwSetFramebufferSizeCallback(mPtr, framebufferSizeCallback);
            //glfwSetWindowSizeCallback(mPtr, framebufferSizeCallback);
            //glfwSetCursorPosCallback(m_ptr, Mouse::cursor_callback);
            //glfwSetMouseButtonCallback(m_ptr, Mouse::button_callback);
            //glfwSetScrollCallback(m_ptr, Mouse::scroll_callback);
            //glfwSetKeyCallback(m_ptr, Keyboard::key_callback);
            return true;
		}
        void swapBuffers() {
            glfwSwapBuffers(mPtr);
        }
        void pollEvents() {
            glfwPollEvents();
        }
        void clearBuffers() const {
            glClearColor(mClearColor[0], mClearColor[1], mClearColor[2], 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }
        void close() {
            mClosed = true;
        }
        void terminate() {
            glfwTerminate();
        }
        void setClearColor(glm::vec3 tColor) {
            mClearColor = tColor;
        }
        void setVSync(bool tVSync) {
            mVSync = tVSync;
            if (mVSync) glfwSwapInterval(1);
            else glfwSwapInterval(0);
        }
        void toggleVSync() { setVSync(!mVSync); }
        bool getVSync() const { return mVSync; }

        bool isOpen() const { return (!mClosed && !glfwWindowShouldClose(mPtr)); }
        int getHeight() const { return mHeight; }
        int getWidth() const { return mWidth; }
        glm::vec2 getSize() const { return glm::vec2(mWidth, mHeight); }
        float aspect() const { return static_cast<float>(mWidth) / static_cast<float>(mHeight); }
        GLFWwindow* ptr() const { return mPtr; }

        enum Cursor {
            CUR_NORMAL = 0x0,
            CUR_CAPTURED = 0x1,
            CUR_HIDDEN = 0x2,
            CUR_DISABLED = 0x3,
            CUR_UNAVAILABLE = 0x4
        };

        void setCursorMode(Cursor tMode) {
            switch (tMode) {
            case Window::CUR_CAPTURED:
                glfwSetInputMode(mPtr, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
                break;
            case Window::CUR_HIDDEN:
                glfwSetInputMode(mPtr, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                break;
            case Window::CUR_DISABLED:
                glfwSetInputMode(mPtr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                break;
            case Window::CUR_UNAVAILABLE:
                glfwSetInputMode(mPtr, GLFW_CURSOR, GLFW_CURSOR_UNAVAILABLE);
                break;
            default:
                glfwSetInputMode(mPtr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                break;
            }
        }
	private:
		GLFWwindow* mPtr;
		int mWidth, mHeight;
		bool mVSync, mClosed;
		glm::vec3 mClearColor;
    private:
        //Resize viewport.
        static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
            glViewport(0, 0, width, height);
        }
        static void errorCallback(int tEC, const char* tDescription) {
            LOG_ERRR("GLFW Error(", std::to_string(tEC).c_str(), "): ", tDescription);
        }
	};
}
#endif // WINDOW_H