#pragma once

struct GLFWwindow;
#ifdef _WIN32
using NativeWindowHandle = struct HWND__*;
#endif

class Window {
public:
    Window(u32 _width, u32 _height, const char* _name);
    ~Window();

    bool shouldClose();
    void pollEvents();

    uint2 getDims() const { return m_dims; }
    void setDims(uint2 _newDims) { m_dims = _newDims; };

    void setFullscreen(bool _isFullscreen) { m_isFullscreen = _isFullscreen; }
    bool isFullscreen() { return m_isFullscreen; }
    bool isMinimized();

    GLFWwindow* getGlfwHandle() const { return m_window; }
    NativeWindowHandle getNativeHandle();


    uint2 m_windowedDims;
    uint2 m_windowedPos;

private:
    GLFWwindow* m_window;
    uint2 m_dims;
    bool m_isFullscreen = false;
    bool m_shouldClose = false;
};