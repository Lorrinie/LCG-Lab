#include "VMRenderFramework.hpp"

VMRenderFramework renderFramework = VMRenderFramework("VirtualMemory", 1920, 1080);

int main() {
	renderFramework.initialize();
	renderFramework.showWindow();
	renderFramework.runLoop();
}