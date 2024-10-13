#ifndef FS_SCENE_H
#define FS_SCENE_H

#include "../entity.hpp"

namespace Firesteel {
	struct Scene {
		std::vector<Entity> entities;

		void load(const std::string& tPath) {

		}
	};
}

#endif // !FS_SCENE_H