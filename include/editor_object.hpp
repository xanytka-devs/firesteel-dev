#include <../engine/include/entity.hpp>
#include "material.hpp"

struct EditorObject {
	std::string name;
	Entity entity;
	Material material;

	void draw() {
		entity.draw(&material.shader);
	}

	void remove() {
		entity.clearMeshes();
	}
};