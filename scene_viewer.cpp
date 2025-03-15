#include "engine/include/app.hpp"
#include "engine/include/utils/json.hpp"
#include "engine/include/camera.hpp"
#include "engine/external/imgui/backends/imgui_impl_opengl3.h"
#include "engine/external/imgui/backends/imgui_impl_glfw.h"
#include "engine/external/imgui/imgui.h"

#include "include/utils.hpp"
#include "include/editor_object.hpp"
#include "include/scene.hpp"
#include "include/embeded.hpp"
using namespace Firesteel;

class SceneViewerApp : public App {
	std::vector<Material> materialReg;
	std::vector<std::string> materialTypes;

	Shader defaultShader;
	Cubemap sky;
	Scene scene;
	Camera camera{ glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0, 0, -90) };
	EditorObject dobj;

	virtual void onInitialize() override {
		// Initialization point.
		window.setClearColor(glm::vec3(0));
		defaultShader = Shader(emb::shd_vrt_default, emb::shd_frg_default, false, "");
		// Load all default materials.
		LOG_INFO("Initializing shaders");
		materialTypes.push_back("Lit");
		materialTypes.push_back("Unlit");
		materialReg.push_back(Material().load("res/mats/text.material.json"));
		materialReg.push_back(Material().load("res/mats/billboard.material.json"));
		materialReg.push_back(Material().load("res/mats/particle.material.json"));
		materialReg.push_back(Material().load("res/mats/skybox.material.json"));
		materialReg.push_back(Material().load("res/mats/lit_3d.material.json"));
		for (size_t i = 0; i < materialReg.size(); i++) {
			materialReg[i].reload();
			if (!materialReg[i].isLoaded()) materialReg[i].shader = defaultShader;
		}
		// Scene & default obj.
		dobj = EditorObject{ "Box", Entity("res\\box\\box.obj", glm::vec3(-2.f, 0.f, 1.f)), materialReg[4] };
		scene = Scene("res\\demo.scene.json", &sky);
		camera.farPlane = 1000;
		// ImGui setup.
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_::ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard
			| ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_::ImGuiConfigFlags_ViewportsEnable;
		// Setup Dear ImGui style
		ImGui::StyleColorsDark(); // you can also use ImGui::StyleColorsClassic();
		// Choose backend
		ImGui_ImplGlfw_InitForOpenGL(window.ptr(), true);
		ImGui_ImplOpenGL3_Init();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
	}
	virtual void onUpdate() override {
		// Handle window size change.
		camera.aspect = window.aspect();
		glViewport(0, 0, static_cast<GLsizei>(window.getWidth()), static_cast<GLsizei>(window.getHeight()));
		window.clearBuffers();
		// Get matrixes ready.
		glm::mat4 projection = camera.getProjection(1);
		glm::mat4 view = camera.getView();
		glm::mat4 model = glm::mat4(1.0f);
		/* Prerender processing */ {
			for (size_t i = 0; i < materialReg.size(); i++) {
				materialReg[i].enable();
				materialReg[i].setMat4("projection", projection);
				materialReg[i].setMat4("view", view);
				materialReg[i].setVec3("viewPos", camera.pos);
				materialReg[i].setInt("drawMode", 0);
				materialReg[i].setInt("time", static_cast<int>(glfwGetTime()));
				if (materialReg[i].type == 0) {
					scene.atmosphere.setParams(&materialReg[i].shader);
					materialReg[i].setInt("lightingType", 3);
					materialReg[i].setInt("skybox", 11);
					//materialReg[i].setInt("numPointLights", 0);
					//pLight.setParams(&materialReg[i].shader, 0);
					//materialReg[i].setInt("numSpotLights", 0);
					//sLight.setParams(&materialReg[i].shader, 0);
				}
			}
			sky.bind();
			defaultShader.enable();
			defaultShader.setVec2("resolution", window.getSize());
		}
		sky.draw(&materialReg[3].shader);
		// Draw default obj.
		dobj.draw();
	}
	virtual void onShutdown() override {
		// Terminate OGL.
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		LOG_INFO("ImGui terminated");
		// Materials & objects
		for (size_t i = 0; i < scene.entities.size(); i++)
			scene[static_cast<int>(i)].remove();
		scene.entities.clear();
		for (size_t i = 0; i < materialReg.size(); i++)
			materialReg[static_cast<int>(i)].remove();
		materialReg.clear();
	}
};

int main() {
	SceneViewerApp app{};
	int r = app.start(("Firesteel " + GLOBAL_VER).c_str(), 800, 600, WS_NORMAL);
	LOG_INFO("Shutting down Firesteel App.");
	return r;
}