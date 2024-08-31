#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>

namespace LearningOpenGL {
	struct PointLight {
		glm::vec3 position;
		glm::vec3 color;
	};
}

#endif // LIGHT_H