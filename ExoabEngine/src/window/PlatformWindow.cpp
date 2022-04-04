#include "PlatformWindow.hpp"
#include "../utils/MBox.hpp"
#include <GLFW/glfw3.h>
#include <vector>
#include <atomic>
#include <unordered_map>

#if defined(_DEBUG)
constexpr bool DebugEnabled = true;
#else
constexpr bool DebugEnabled = false;
#endif

static std::unordered_map<GLFWwindow*, PlatformWindow*> s_Internal_Windows;

static PlatformWindow *_Internal_GetWindow(GLFWwindow *window)
{
    auto it = s_Internal_Windows.find(window);
    if (it != s_Internal_Windows.end()) {
        return it->second;
    }
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

void _Internal_WindowMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    PlatformWindow* w = _Internal_GetWindow(window);
    if (!w)
        return;
    for (auto& callback : w->mCallbacks) {
        if (callback.first & EVENT_MOUSE_PRESS || callback.first & EVENT_MOUSE_RELEASE) {
            Event e{};
            e.mEvents = (action == GLFW_PRESS) ? EVENT_MOUSE_PRESS : EVENT_MOUSE_RELEASE;
            switch (button) {
                case GLFW_MOUSE_BUTTON_LEFT:
                    e.mDetails = EVENT_DETAIL_LEFT_BUTTON;
                    break;
                case GLFW_MOUSE_BUTTON_MIDDLE:
                    e.mDetails = EVENT_DETAIL_MIDDLE_BUTTON;
                    break;
                case GLFW_MOUSE_BUTTON_RIGHT:
                    e.mDetails = EVENT_DETAIL_RIGHT_BUTTON;
                    break;
                default:
                    printf("Unknowning mouse button pressed!\n");
                    return;
            }
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            e.mPayload.mClickX = int(xpos);
            e.mPayload.mClickY = int(ypos);
            callback.second(e);
        }
    }
}

PlatformWindow::PlatformWindow(std::string title, int width, int height)
    : m_width(width), m_height(height)
{
    memset(m_keys, 3, 512 * 4);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    glfwSetWindowSizeCallback(m_window, _Internal_WindowResizeCallback);
    glfwSetWindowFocusCallback(m_window, _Internal_WindowFocusCallback);
    glfwSetKeyCallback(m_window, _Internal_WindowKeyCallback);
    glfwSetMouseButtonCallback(m_window, _Internal_WindowMouseButtonCallback);
    s_Internal_Windows[m_window] = this;
}

PlatformWindow::~PlatformWindow()
{
    s_Internal_Windows.erase(m_window);
    glfwDestroyWindow(m_window);
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

void PlatformWindow::RegisterCallback(EventFlagBits events, const std::function<void(const Event& e)>& func) {
    mCallbacks.push_back(std::make_pair(events, func));
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
