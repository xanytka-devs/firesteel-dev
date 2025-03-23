#include "engine/include/app.hpp"
using namespace Firesteel;

class ThreeDExampleApp : public App {

};

int main() {
	ThreeDExampleApp app{};
	int r = app.start("3D Example ", 800, 600, WS_NORMAL);
	LOG_INFO("Shutting down Firesteel App.");
	return r;
}