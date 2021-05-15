#include "common.h"

#include "globals.h"
#include "logging/logger.h"
#include "window/window.h"
#include "rendering/driver.h"
#include "rendering/renderer.h"
#include "rendering/shader.h"

namespace globals {

    Logger* logger;
    Window* window;
    Renderer* renderer;
    Driver* driver;
    ShaderManager* shadermgr;

    void init()
    {
        logger = new Logger;
        globals::GlobalObject<Logger>::set(logger);

        window = new Window(1280, 720, APP_NAME);
        globals::GlobalObject<Window>::set(window);

        driver = new Driver(*window);
        globals::GlobalObject<Driver>::set(driver);

        shadermgr = new ShaderManager();
        globals::GlobalObject<ShaderManager>::set(shadermgr);

        renderer = new Renderer(*window);
        globals::GlobalObject<Renderer>::set(renderer);
    }

    void deinit()
    {
        delete renderer;
        delete shadermgr;
        delete driver;
        delete window;
        delete logger;
    }
}
