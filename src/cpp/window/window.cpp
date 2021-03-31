#include "common.h"

#include "window.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// https://stackoverflow.com/a/31526753/2675758
GLFWmonitor* getCurrentMonitor(GLFWwindow* window)
{
    int nmonitors, i;
    int wx, wy, ww, wh;
    int mx, my, mw, mh;
    int overlap, bestoverlap;
    GLFWmonitor* bestmonitor;
    GLFWmonitor** monitors;
    const GLFWvidmode* mode;

    bestoverlap = 0;
    bestmonitor = NULL;

    glfwGetWindowPos(window, &wx, &wy);
    glfwGetWindowSize(window, &ww, &wh);
    monitors = glfwGetMonitors(&nmonitors);

    for (i = 0; i < nmonitors; i++) {
        mode = glfwGetVideoMode(monitors[i]);
        glfwGetMonitorPos(monitors[i], &mx, &my);
        mw = mode->width;
        mh = mode->height;

        overlap = max(0, min(wx + ww, mx + mw) - max(wx, mx)) * max(0, min(wy + wh, my + mh) - max(wy, my));

        if (bestoverlap < overlap) {
            bestoverlap = overlap;
            bestmonitor = monitors[i];
        }
    }

    return bestmonitor;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    UNREFERENCED_PARAMETER(mods);
    UNREFERENCED_PARAMETER(scancode);

    if (action != GLFW_PRESS)
        return;

    Window* windowPtr = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, true);
    
    if (mods == GLFW_MOD_ALT) {
        if (key == GLFW_KEY_ENTER) {
            if (!windowPtr->isFullscreen()) {
                windowPtr->setFullscreen(true);
                windowPtr->m_windowedDims = windowPtr->getDims();
                glfwGetWindowPos(window, (int*)&windowPtr->m_windowedPos.x, (int*)&windowPtr->m_windowedPos.y);
                const GLFWvidmode* mode = glfwGetVideoMode(getCurrentMonitor(window));
                glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
                glfwSetWindowPos(window, 0, 0);
                glfwSetWindowSize(window, mode->width, mode->height);
            } else {
                windowPtr->setFullscreen(false);
                windowPtr->setDims(windowPtr->m_windowedDims);
                glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
                glfwSetWindowSize(window, windowPtr->m_windowedDims.x, windowPtr->m_windowedDims.y);
                glfwSetWindowPos(window, windowPtr->m_windowedPos.x, windowPtr->m_windowedPos.y);
            }
        }
    }
}

void resizeCallback(GLFWwindow* _myWindow, int _width, int _height)
{
    Window* windowPtr = static_cast<Window*>(glfwGetWindowUserPointer(_myWindow));
    uint2 newDims = { (u32)_width, (u32)_height };
    windowPtr->setDims(newDims);
    //logInfo("Window resize to {}x{}", newDims.x, newDims.y);
}


constexpr u32 WINDOW_MIN_WIDTH = 128;
constexpr u32 WINDOW_MIN_HEIGHT = WINDOW_MIN_WIDTH * 9 / 16;
Window::Window(u32 _width, u32 _height, const char* _name)
    : m_windowedPos {}
    , m_windowedDims {}
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(_width, _height, _name, nullptr, nullptr);
    glfwSetWindowSizeLimits(m_window, WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);

    m_dims = { _width, _height };

    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetWindowSizeCallback(m_window, resizeCallback);
}

bool Window::isMinimized()
{
    return m_dims.x < WINDOW_MIN_WIDTH && m_dims.y < WINDOW_MIN_HEIGHT;
}

NativeWindowHandle Window::getNativeHandle()
{
    #ifdef _WIN32
    return glfwGetWin32Window(m_window);
    #else
    #error Not implemented
    #endif
}

Window::~Window()
{
    glfwDestroyWindow(m_window);

    glfwTerminate();
}

bool Window::shouldClose()
{
    return glfwWindowShouldClose(m_window);
}

void Window::pollEvents()
{
    glfwPollEvents();
}
