#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>

namespace LearningOpenGL {
	struct DirectionalLight {
		glm::vec3 direction;
		// Lighting params.
		glm::vec3 ambient = glm::vec3(0.2f);
		glm::vec3 diffuse = glm::vec3(1);
		glm::vec3 specular = glm::vec3(0.5f);
	};
	struct PointLight {
		glm::vec3 position;
		// Lighting params.
		glm::vec3 ambient = glm::vec3(0.2f);
		glm::vec3 diffuse = glm::vec3(1);
		glm::vec3 specular = glm::vec3(0.5f);
		// Attenuation.
		float constant = 1.f;
		float linear = 0.09f;
		float quadratic = 0.032f;
	};
	struct SpotLight {
		glm::vec3 position;
		glm::vec3 direction;
		// Lighting params.
		glm::vec3 ambient = glm::vec3(0.2f);
		glm::vec3 diffuse = glm::vec3(1);
		glm::vec3 specular = glm::vec3(0.5f);
		// Attenuation.
		float cutOff = glm::cos(glm::radians(12.5f));
		float outerCutOff = glm::cos(glm::radians(17.5f));
		float constant = 1.f;
		float linear = 0.09f;
		float quadratic = 0.032f;
	};
}

#endif // LIGHT_H