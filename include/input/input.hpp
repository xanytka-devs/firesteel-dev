#ifndef FS_INPUT_H
#define FS_INPUT_H

#include <../include/input/keyboard.hpp>
#include <../include/input/mouse.hpp>
#include <../include/input/joystick.hpp>

namespace LearningOpenGL {
	class Input {
	public:
		static float get_horizontal_axis() {
			if (Keyboard::key_state(KeyCode::D)) return 1.f;
			if (Keyboard::key_state(KeyCode::A)) return -1.f;
			return 0.f;
		}
		static float get_vertical_axis() {
			if (Keyboard::key_state(KeyCode::W)) return 1.f;
			if (Keyboard::key_state(KeyCode::S)) return -1.f;
			return 0.f;
		}
	};
}

#endif // !FS_INPUT_H