#include <../include/input/input.hpp>
using namespace Firesteel;

bool Keyboard::m_keys[GLFW_KEY_LAST] = { 0 };
bool Keyboard::m_keys_changed[GLFW_KEY_LAST] = { 0 };

float Mouse::mX = 0;
float Mouse::mY = 0;
float Mouse::m_old_x = 0;
float Mouse::m_old_y = 0;

float Mouse::mDX = 0;
float Mouse::mDY = 0;

float Mouse::m_wheel_dx = 0;
float Mouse::m_wheel_dy = 0;

bool Mouse::m_first_move = true;
bool Mouse::m_buttons[GLFW_MOUSE_BUTTON_LAST] = { 0 };
bool Mouse::m_buttons_changed[GLFW_MOUSE_BUTTON_LAST] = { 0 };



#include <../include/utils/log.hpp>

#ifdef WIN32
#include <windows.h>
void Log::log(const std::string& tMsg, const int t_mod) {
	SetConsoleTitleA("firesteel Debug Output"); //Set cmd title.
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);//Get cmd handle.
	SetConsoleTextAttribute(hConsole, t_mod); //Set cmd text color.
	printf(tMsg.c_str()); //Print msg.
}
void Log::clear() {
	system("cls");
}
#endif // WIN32