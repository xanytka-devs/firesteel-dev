#ifndef FS_APP_H
#define FS_APP_H

#include "common.hpp"
#include "window.hpp"
#include "renderer.hpp"

namespace Firesteel {

	class App {
    private:
        double mLastFrameFPS = 0, mLastFrame = 0;
        int mFrameCount = 0;
	public:
		App() {
			LOG_INFO("Initializing Firesteel App.");
		}
		virtual ~App() {
			LOG_INFO("Shutting down Firesteel App.");
		}
		void shutdown() {
			window.close();
		}
		virtual int start(const char* tTitle, unsigned int tWinWidth, unsigned int tWinHeight, bool tFullscreen) {
            LOG("Firesteel 0.2.0.3");
            LOG_C("[-   Dev branch  -]\n", CMD_F_PURPLE);
            LOG_STATE("STARTUP");
			onPreInitialize();
            // Create window.
            window = Window(tWinHeight, tWinWidth);
            if(!window.initialize(tTitle, tFullscreen, BOUND_GL_VERSION_MAJOR, BOUND_GL_VERSION_MINOR))
                return 1;
            window.setClearColor(glm::vec3(0.185f, 0.15f, 0.1f));

            printf("\n");
            //Check for Vulkan.
            bool isVulkan = (glfwVulkanSupported() == 1);
            LOG_INFO(isVulkan ? "Vulkan is supported on current machine."
                : "Vulkan isn't supported on current machine.");
            if(isVulkan) {
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
            Renderer r = Renderer();
            if (!r.initialize()) {
                LOG_ERRR("OpenGL isn't supported on current machine.");
                return -1;
            }
            else LOG_INFO("OpenGL is supported on current machine.");
            r.loadExtencions();
            r.printInfo();
            r.initializeParams();
            onInitialize();

            // Update loop.
            LOG_STATE("UPDATE LOOP");
            while (window.isOpen()) {
                window.pollEvents();
                //Per-frame time logic.
                double currentFrame = glfwGetTime();
                deltaTime = currentFrame - mLastFrame;
                mLastFrame = currentFrame;
                mFrameCount++;
                if (currentFrame - mLastFrameFPS >= 1.0) {
                    fps = mFrameCount;
                    mFrameCount = 0;
                    mLastFrameFPS = currentFrame;
                }
                onUpdate();
                window.swapBuffers();
            }
            LOG_STATE("SHUTDOWN");
            onShutdown();
            // Quitting.
            window.terminate();
            LOG_INFO("Window terminated");
            LOG_STATE("QUIT");
            return 0;
		}
		virtual void onPreInitialize() { }
		virtual void onInitialize() { }
		virtual void onUpdate() { }
		virtual void onShutdown() { }

		Window window;
		unsigned int fps = 0;
        float deltaTime = 0.0f;
	};

}

#endif // !FS_APP_H
