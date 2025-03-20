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
#include "include/imgui/base.hpp"
using namespace Firesteel;

class SceneViewerApp : public App {
	std::vector<Material> materialReg;
	std::vector<std::string> materialTypes;

	Shader defaultShader;
	Cubemap sky;
	Scene scene;
	Camera camera{ glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0, 0, -90) };

	float speed_mult = 2.f, mouseSenstivity = 0.5f;
	bool mouseInvertY = false;
	std::filesystem::path lastPath;
	void processInput(Window* tWin, float tDeltaTime) {
		if (Keyboard::keyDown(KeyCode::ESCAPE))
			tWin->close();

		if ((Keyboard::getKey(KeyCode::LEFT_CONTROL) || Keyboard::getKey(KeyCode::RIGHT_CONTROL)) && !Mouse::getButton(1)) {
			camera.fov -= Mouse::getWheelDY();
			if (camera.fov < 1.0f)
				camera.fov = 1.0f;
			if (camera.fov > 100.0f)
				camera.fov = 100.0f;
		}

		if (!Mouse::getButton(1)) tWin->setCursorMode(Window::CUR_NORMAL);
		else tWin->setCursorMode(Window::CUR_DISABLED);

		if (!Mouse::getButton(1)) return;
		camera.pos += camera.Forward * (tDeltaTime * Mouse::getWheelDY() * 10.f);

		float dx = Mouse::getCursorDX(), dy = Mouse::getCursorDY();
		if (dx != 0 || dy != 0) {
			if (mouseInvertY) dy *= -1;
			camera.rot.y += dy * mouseSenstivity;
			camera.rot.z += dx * mouseSenstivity;
			if (camera.rot.y > 89.0f)
				camera.rot.y = 89.0f;
			if (camera.rot.y < -89.0f)
				camera.rot.y = -89.0f;
			if (camera.rot.z >= 360.0f)
				camera.rot.z -= 360.0f;
			if (camera.rot.z <= -360.0f)
				camera.rot.z += 360.0f;
			camera.update();
		}

		float velocity = 2.5f * tDeltaTime * speed_mult;
		bool isPressingMoveKey = false;

		if (Keyboard::keyDown(KeyCode::M))
			tWin->toggleVSync();

		if (Keyboard::getKey(KeyCode::W)) {
			camera.pos += camera.Forward * velocity;
			isPressingMoveKey = true;
		}
		if (Keyboard::getKey(KeyCode::S)) {
			camera.pos -= camera.Forward * velocity;
			isPressingMoveKey = true;
		}
		if (Keyboard::getKey(KeyCode::A)) {
			camera.pos -= camera.Right * velocity;
			isPressingMoveKey = true;
		}
		if (Keyboard::getKey(KeyCode::D)) {
			camera.pos += camera.Right * velocity;
			isPressingMoveKey = true;
		}

		if (Keyboard::getKey(KeyCode::LEFT_SHIFT) && isPressingMoveKey)
			speed_mult += 0.005f;
		else speed_mult = 1.f;

		if (Keyboard::getKey(KeyCode::SPACEBAR))
			camera.pos.y += velocity;
		if (Keyboard::getKey(KeyCode::LEFT_CONTROL))
			camera.pos.y -= velocity;
	}

	virtual void onInitialize() override {
		// Initialization point.
		lastPath = std::filesystem::current_path();
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
		// Scene.
		//scene.load("res\\demo.scene.json", &sky);
		camera.farPlane = 1000;
		// ImGui setup.
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		// Setup Dear ImGui style
		ImGui::StyleColorsDark(); // you can also use ImGui::StyleColorsClassic();
		// Choose backend
		ImGui_ImplGlfw_InitForOpenGL(window.ptr(), true);
		ImGui_ImplOpenGL3_Init();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
		ImGui::LoadIniSettingsFromDisk("viewer.imgui.ini");
		std::filesystem::remove("imgui.ini");
	}
	virtual void onUpdate() override {
		// Handle window size change.
		camera.aspect = window.aspect();
		glViewport(0, 0, static_cast<GLsizei>(window.getWidth()), static_cast<GLsizei>(window.getHeight()));
		processInput(&window, deltaTime);
		window.clearBuffers();
		// Get matrixes ready.
		glm::mat4 projection = camera.getProjection(1);
		glm::mat4 view = camera.getView();
		glm::mat4 model = glm::mat4(1.0f);
		defaultShader.enable();
		defaultShader.setVec2("resolution", window.getSize());
		scene.draw(&camera, &materialReg, &sky);
		// Start the Dear ImGui frame
		FSImGui::NewFrame();
		// General window.
		ImGui::Begin("Scene viewer");
		ImGui::Text(("FPS " + std::to_string(fps)).c_str());

		ImGui::Text("Scene");
		if(scene.isLoaded()) {
			ImGui::BeginDisabled();
			std::string cPath = scene.getPath();
			ImGui::InputText("##scene_curpath", &cPath);
			ImGui::EndDisabled();
		} else ImGui::Text("No scene");
		ImGui::SameLine();
		if(ImGui::Button("...")) {
			FileDialog fd;
			fd.filter = "All\0*.*\0Scene (*.scene.json)\0*.scene.json\0";
			fd.filter_id = 2;
			std::string res = fd.open();
			if (res != "") {
				LOG_INFO("Opening scene at: \"" + res + "\".");
				std::filesystem::current_path(std::filesystem::current_path().parent_path());
				scene.load(res.c_str(), &sky);
				std::filesystem::current_path(lastPath);
				LOG_INFO("Scene succesfully loaded.");
			}
		}

		ImGui::Text("Mouse settings");
		ImGui::SliderFloat("Sensetivity", &mouseSenstivity, 0.01f, 2.f);
		FSImGui::DragFloat3("Camera pos", &camera.pos);

		ImGui::End();
		// End frame.
		FSImGui::Render(&window);
	}
	virtual void onShutdown() override {
		ImGui::SaveIniSettingsToDisk("viewer.imgui.ini");
		// Terminate OGL.
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		LOG_INFO("ImGui terminated");
		// Materials & objects.
		scene.remove();
		for (size_t i = 0; i < materialReg.size(); i++)
			materialReg[static_cast<int>(i)].remove();
		materialReg.clear();
	}
};

int main() {
	SceneViewerApp app{};
	int r = app.start(("Scene Viewer | Firesteel " + GLOBAL_VER).c_str(), 800, 600, WS_NORMAL);
	LOG_INFO("Shutting down Firesteel App.");
	return r;
}