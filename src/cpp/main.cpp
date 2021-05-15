#include "common.h"

#include "logging/logger.h"
#include "window/window.h"
#include "rendering/renderer.h"


int main()
{
    globals::init();
    logInfo("ooo!");

    while (!globals::getPtr<Window>()->shouldClose()) 
    {
        globals::getPtr<Window>()->pollEvents();
        globals::getPtr<Renderer>()->RenderFrame();
    }

    globals::deinit();
	return 0;
}

#ifdef _WIN32
#include <Windows.h>
INT WinMain(HINSTANCE, HINSTANCE, PSTR, INT)
{
    return main();
}
#endif