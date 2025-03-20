#include <../engine/include/entity.hpp>
#include "material.hpp"

struct EditorObject {
	std::string name;
	Entity entity;
	Material material;
	unsigned int materialID = 0;

	void draw() {
		entity.draw(&material.shader);
	}

	void draw(Material* tMat) {
		entity.draw(&tMat->shader);
	}

	void remove() {
		entity.clearMeshes();
	}
};