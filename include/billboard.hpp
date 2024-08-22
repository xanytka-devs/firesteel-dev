#ifndef BILLBOARD_H
#define BILLBOARD_H

#include "common.hpp"

namespace LearningOpenGL {

	class Billboard {
	public:
		Billboard();
		~Billboard();

		void draw();

		glm::vec3 position;
	private:
		GLuint texture;

		GLuint VAO, VBO;
	};

}

#endif // BILLBOARD_H