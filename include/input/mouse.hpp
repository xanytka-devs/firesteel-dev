#ifndef FS_MOUSE_H
#define FS_MOUSE_H

#include <../include/common.hpp>

namespace LearningOpenGL {
	class Mouse {
	public:
		/// Set mouse cursor position.
		static void CursorCallback(GLFWwindow* t_window, double t_x, double t_y) {
			mX = static_cast<float>(t_x);
			mY = static_cast<float>(t_y);
			//Check if first mouse button.
			if (m_first_move) {
				m_old_x = static_cast<float>(t_x);
				m_old_y = static_cast<float>(t_y);
				m_first_move = false;
			}
			//Set deltas.
			mDX = mX - m_old_x;
			mDY = m_old_y - mY;
			//Set old positions
			m_old_x = mX;
			m_old_y = mY;
		}
		/// Set mouse buttons statuses.
		static void ButtonCallback(GLFWwindow* t_window, int t_button, int t_action, int t_mods) {
			//Check action.
			if (t_action != GLFW_RELEASE) {
				if (!m_buttons[t_button]) m_buttons[t_button] = true;
			}
			else m_buttons[t_button] = false;
			//Detect if button is pressed continuously.
			m_buttons[t_button] = t_action != GLFW_REPEAT;
		}
		/// Set mouse wheel deltas.
		static void ScrollCallback(GLFWwindow* t_window, double t_dx, double t_dy) {
			m_wheel_dx = static_cast<float>(t_dx);
			m_wheel_dy = static_cast<float>(t_dy);
		}

		/// Get cursor x.
		static float getCursorX() { return mX; }
		/// Get cursor y.
		static float getCursorY() { return mY; }

		/// Get cursor x delta.
		static float getCursorDX() {
			float _dx = mDX;
			mDX = 0;
			return _dx;
		}
		/// Get cursor y delta.
		static float getCursorDY() {
			float _dy = mDY;
			mDY = 0;
			return _dy;
		}

		/// Get mouse wheel x delta.
		static float getWheelDX() {
			float w_dx = m_wheel_dx;
			m_wheel_dx = 0;
			return w_dx;
		}
		/// Get mouse wheel y delta.
		static float getWheelDY() {
			float w_dy = m_wheel_dy;
			m_wheel_dy = 0;
			return w_dy;
		}

		/// Gets current status of mouse button.
		static bool buttonState(int t_button) {
			return m_buttons[t_button];
		}
		/// Has mouse button changed?
		static bool buttonChanged(int t_button) {
			bool output = m_buttons[t_button];
			m_buttons[t_button] = false;
			return output;
		}
		/// Is mouse button up?
		static bool buttonUp(int t_button) {
			return !m_buttons[t_button] && buttonChanged(t_button);
		}
		/// Is mouse button down?
		static bool buttonDown(int t_button) {
			return m_buttons[t_button] && buttonChanged(t_button);
		}
	private:
		static float mX, mY, m_old_x, m_old_y;

		static float mDX, mDY;

		static float m_wheel_dx, m_wheel_dy;

		static bool m_first_move;
		static bool m_buttons[];
		static bool m_buttons_changed[];
	};
}

#endif // !FS_MOUSE_H