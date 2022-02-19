#include "PlatformWindow.hpp"
#include "../utils/MBox.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <atomic>

#if defined(_DEBUG)
constexpr bool DebugEnabled = true;
#else
constexpr bool DebugEnabled = false;
#endif

static std::vector<PlatformWindow *> s_Internal_Windows;
static std::atomic<uint32_t> s_Internal_WindowCount(0);

static PlatformWindow *_Internal_GetWindow(GLFWwindow *window)
{
    for (PlatformWindow *w : s_Internal_Windows)
        if (w->GetWindow() == window)
            return w;
    return nullptr;
}

static void _Internal_WindowResizeCallback(GLFWwindow *window, int width, int height)
{
    PlatformWindow *w = _Internal_GetWindow(window);
    if (!w)
        return;
    if (width != 0 && height != 0) {
        w->m_width = width;
        w->m_height = height;
    }
}

static void _Internal_WindowKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    PlatformWindow *w = _Internal_GetWindow(window);
    if (!w)
        return;
    if (action == GLFW_REPEAT)
        action = GLFW_PRESS;
    w->m_keys[key] = action;
}

void _Internal_WindowFocusCallback(GLFWwindow *window, int focused)
{
    PlatformWindow *w = _Internal_GetWindow(window);
    if (!w)
        return;
    w->m_focus = focused;
}

PlatformWindow::PlatformWindow(std::string title, int width, int height)
    : m_width(width), m_height(height)
{
    memset(m_keys, 3, 512 * 4);
    if (s_Internal_WindowCount == 0)
    {
        if (glfwInit() != GLFW_TRUE)
        {
            Utils::Message("Fatal Error", "GLFW Library could not initalize!", Utils::ERR);
            exit(1);
        }
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    glfwSetWindowSizeCallback(m_window, _Internal_WindowResizeCallback);
    glfwSetWindowFocusCallback(m_window, _Internal_WindowFocusCallback);
    glfwSetKeyCallback(m_window, _Internal_WindowKeyCallback);
    s_Internal_Windows.push_back(this);
    s_Internal_WindowCount++;
}

PlatformWindow::~PlatformWindow()
{
    int index = 0;
    for (PlatformWindow *w : s_Internal_Windows)
    {
        if (w->GetWindow() == m_window)
        {
            s_Internal_Windows.erase(s_Internal_Windows.begin() + index);
            break;
        }
        index++;
    }
    glfwDestroyWindow(m_window);
    s_Internal_WindowCount--;
    if (s_Internal_WindowCount == 0)
        glfwTerminate();
}

void PlatformWindow::Poll()
{
    glfwPollEvents();
}

bool PlatformWindow::ShouldClose()
{
    return glfwWindowShouldClose(m_window);
}

bool PlatformWindow::IsWindowMinimized()
{
    int w, h;
    glfwGetWindowSize(m_window, &w, &h);
    return !(w && h);
}

bool PlatformWindow::IsKeyDown(uint16_t key)
{
    key = toupper(key);
    if (m_keys[key] == GLFW_PRESS)
        return true;
    return false;
}

bool PlatformWindow::IsKeyUp(uint16_t key)
{
    key = toupper(key);
    bool status = m_keys[key] == GLFW_RELEASE;
    m_keys[key] = 3;
    return status;
}
