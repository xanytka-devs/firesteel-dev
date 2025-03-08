#ifndef FSE_SCENE
#define FSE_SCENE

#include "../engine/include/common.hpp"
#include "../engine/include/utils/json.hpp"
#include <fstream>
#include "atmosphere.hpp"
#include "utils.hpp"
using namespace Firesteel;

class Scene {
public:
	Scene() {

	}
	Scene(const char* tPath, Cubemap* tSky) {
		mLoaded = loadFromSrc(tPath, tSky);
	}

	void save(const char* tPath) {
		nlohmann::json txt;

		txt["version"] = 1;

		glm::vec3 v = atmosphere.directionalLight.ambient;
		txt["atm"]["dLight"]["amb"] = { v.x, v.y, v.z };
		v = atmosphere.directionalLight.diffuse;
		txt["atm"]["dLight"]["diff"] = { v.x, v.y, v.z };
		v = atmosphere.directionalLight.direction;
		txt["atm"]["dLight"]["dir"] = { v.x, v.y, v.z };
		v = atmosphere.directionalLight.specular;
		txt["atm"]["dLight"]["spec"] = { v.x, v.y, v.z };

		v = atmosphere.fog.color;
		txt["atm"]["fog"]["clr"] = { v.x, v.y, v.z };
		txt["atm"]["fog"]["dnst"] = atmosphere.fog.density;
		txt["atm"]["fog"]["equ"] = atmosphere.fog.equation;
		txt["atm"]["fog"]["points"] = { atmosphere.fog.start, atmosphere.fog.end};

		std::ofstream o(tPath);
		o << std::setw(4) << txt << std::endl;
	}
	EditorObject& operator [](int tID) {
		return *entities[tID];
	}

	void draw() {

	}

	Atmosphere atmosphere;	
	std::vector<std::shared_ptr<EditorObject>> entities;
private:
	bool mLoaded = false;
	bool loadFromSrc(const char* tPath, Cubemap* tSky) {
		if(!std::filesystem::exists(tPath)) return false;
		std::ifstream ifs(tPath);
		nlohmann::json txt = nlohmann::json::parse(ifs);

		atmosphere.directionalLight.ambient = Vec4FromJson(&txt["atm"]["dLight"]["amb"]);
		atmosphere.directionalLight.diffuse = Vec4FromJson(&txt["atm"]["dLight"]["diff"]);
		atmosphere.directionalLight.direction = Vec4FromJson(&txt["atm"]["dLight"]["dir"]);
		atmosphere.directionalLight.specular = Vec4FromJson(&txt["atm"]["dLight"]["spec"]);

		atmosphere.fog.color = Vec4FromJson(&txt["atm"]["fog"]["clr"]);
		atmosphere.fog.density = txt["atm"]["fog"]["dnst"];
		atmosphere.fog.equation = txt["atm"]["fog"]["equ"];
		glm::vec2 pts = Vec4FromJson(&txt["atm"]["fog"]["points"]);
		atmosphere.fog.start = pts.x;
		atmosphere.fog.end = pts.y;

		if(!txt["atm"]["cubemap"].is_null() && std::filesystem::exists(txt["atm"]["cubemap"]["dir"])) {
			tSky->load(txt["atm"]["cubemap"]["dir"],
				txt["atm"]["cubemap"]["posZ"], txt["atm"]["cubemap"]["negZ"],
				txt["atm"]["cubemap"]["posY"], txt["atm"]["cubemap"]["negY"],
				txt["atm"]["cubemap"]["posX"], txt["atm"]["cubemap"]["negX"]);
			tSky->initialize(txt["atm"]["cubemap"]["size"]);
		}

		return true;
	}
};

#endif // !FSE_SCENE
