#include "common.h"

#include "globals.h"
#include "logging/logger.h"
#include "window/window.h"
#include "rendering/renderer.h"

namespace globals {

    Logger* logger;
    Window* window;
    Renderer* renderer;

    void init()
    {
        logger = new Logger;
        globals::GlobalObject<Logger>::set(logger);

        window = new Window(1280, 720, APP_NAME);
        globals::GlobalObject<Window>::set(window);

        renderer = new Renderer(*window);
        globals::GlobalObject<Renderer>::set(renderer);
    }

    void deinit()
    {
        delete renderer;
        delete window;
        delete logger;
    }
}
