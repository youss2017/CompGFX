#include "PlatformWindow.hpp"
#include "../utils/MBox.hpp"
#include <util/utilcore.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <atomic>
#include <unordered_map>

#if defined(_DEBUG)
constexpr bool DebugEnabled = true;
#else
constexpr bool DebugEnabled = false;
#endif
using namespace ut;

namespace egx {

	static std::unordered_map<GLFWwindow*, PlatformWindow*> s_Internal_Windows;

	PlatformWindow* _Internal_GetWindow(GLFWwindow* window)
	{
		auto it = s_Internal_Windows.find(window);
		if (it != s_Internal_Windows.end()) {
			return it->second;
		}
		return nullptr;
	}

	void _Internal_WindowResizeCallback(GLFWwindow* window, int width, int height)
	{
		PlatformWindow* w = _Internal_GetWindow(window);
		if (!w)
			return;
		if (width != 0 && height != 0) {
			w->m_width = width;
			w->m_height = height;
		}
		for (auto& [events, callback] : w->mCallbacks) {
			if (events & EVENT_FLAGS_WINDOW_RESIZE) {
				Event e{};
				e.mEvents = EVENT_FLAGS_WINDOW_RESIZE;
				e.mDetails = EVENT_DETAIL_UNDEFINED;
				e.mPayload.mWidth = width;
				e.mPayload.mHeight = height;
				callback.pFunc(e, callback.pUserDefined);
			}
		}
	}

	void _Internal_WindowKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		PlatformWindow* w = _Internal_GetWindow(window);
		if (!w)
			return;
		if (action == GLFW_REPEAT)
			action = GLFW_PRESS;
		for (auto& [events, callback] : w->mCallbacks) {
			if (events & EVENT_FLAGS_KEY_PRESS || events & EVENT_FLAGS_KEY_RELEASE) {
				if (events & EVENT_FLAGS_KEY_PRESS && action != GLFW_PRESS && !(events & EVENT_FLAGS_KEY_RELEASE))
					continue;
				if (events & EVENT_FLAGS_KEY_RELEASE && action != GLFW_RELEASE && !(events & EVENT_FLAGS_KEY_PRESS))
					continue;
				Event e{};
				e.mEvents = action == GLFW_PRESS ? EVENT_FLAGS_KEY_PRESS : EVENT_FLAGS_KEY_RELEASE;
				e.mDetails = EVENT_DETAIL_UNDEFINED;
				e.mPayload.NonASCIKey = key;
				e.mPayload.KeyLowerCase = tolower(key);
				e.mPayload.KeyUpperCase = toupper(key);
				callback.pFunc(e, callback.pUserDefined);
			}
		}
	}

	void _Internal_WindowFocusCallback(GLFWwindow* window, int focused)
	{
		PlatformWindow* w = _Internal_GetWindow(window);
		if (!w)
			return;
		w->m_focus = focused;
	}

	void _Internal_WindowMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
		PlatformWindow* w = _Internal_GetWindow(window);
		if (!w)
			return;
		for (auto& [events, callback] : w->mCallbacks) {
			if (events & EVENT_FLAGS_MOUSE_PRESS || events & EVENT_FLAGS_MOUSE_RELEASE) {
				if (events & EVENT_FLAGS_MOUSE_PRESS && action != GLFW_PRESS && !(events & EVENT_FLAGS_MOUSE_RELEASE))
					continue;
				if (events & EVENT_FLAGS_MOUSE_RELEASE && action != GLFW_RELEASE && !(events & EVENT_FLAGS_MOUSE_PRESS))
					continue;
				Event e{};
				e.mEvents = action == GLFW_PRESS ? EVENT_FLAGS_MOUSE_PRESS : EVENT_FLAGS_MOUSE_RELEASE;;
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
				callback.pFunc(e, callback.pUserDefined);
			}
		}
	}

	void _Internal_WindowMouseMove(GLFWwindow* window, double xpos, double ypos) {
		PlatformWindow* w = _Internal_GetWindow(window);
		if (!w)
			return;
		for (auto& [events, callback] : w->mCallbacks) {
			if (events & EVENT_FLAGS_MOUSE_MOVE) {
				Event e{};
				e.mEvents = EVENT_FLAGS_MOUSE_MOVE;
				e.mDetails = EVENT_DETAIL_NORMAL_MOTION;
				e.mPayload.mPositionX = (int)xpos;
				e.mPayload.mPositionY = (int)ypos;
				callback.pFunc(e, callback.pUserDefined);
			}
		}
	}

	void _Internal_WindowResize(GLFWwindow* window, int width, int height) {
		PlatformWindow* w = _Internal_GetWindow(window);
		if (!w) return;
	}

	EGX_API PlatformWindow::PlatformWindow(std::string title, int width, int height)
		: m_width(width), m_height(height), nCallbackRemoveOffset(0)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

		glfwSetWindowSizeCallback(m_window, _Internal_WindowResizeCallback);
		glfwSetWindowFocusCallback(m_window, _Internal_WindowFocusCallback);
		glfwSetKeyCallback(m_window, _Internal_WindowKeyCallback);
		glfwSetMouseButtonCallback(m_window, _Internal_WindowMouseButtonCallback);
		glfwSetCursorPosCallback(m_window, _Internal_WindowMouseMove);
		if (!glfwRawMouseMotionSupported()) {
			log_error("RAW INPUT is not supported on your device?");
		}
		else {
			//glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}
		s_Internal_Windows[m_window] = this;
	}

	EGX_API PlatformWindow::~PlatformWindow()
	{
		s_Internal_Windows.erase(m_window);
		glfwDestroyWindow(m_window);
	}

	EGX_API void PlatformWindow::Poll()
	{
		glfwPollEvents();
	}

	EGX_API bool PlatformWindow::ShouldClose()
	{
		return glfwWindowShouldClose(m_window);
	}

	EGX_API bool PlatformWindow::IsWindowMinimized()
	{
		int w, h;
		glfwGetWindowSize(m_window, &w, &h);
		return !(w && h);
	}

	EGX_API int PlatformWindow::RegisterCallback(EventFlagBits events, const std::function<void(const Event& e, void* pUserDefined)>& func, void* pUserDefined) {
		PlatformWindowCallback callback;
		callback.pFunc = func;
		callback.pUserDefined = pUserDefined;
		mCallbacks.push_back(std::make_pair(events, callback));
		return mCallbacks.size() - 1 + nCallbackRemoveOffset;
	}

	EGX_API void PlatformWindow::RemoveCallback(int ID)
	{
		// ID is basically the index in vector, however since we remove elements
		// their indices changes theirfore we use this offset to get the correct index
		mCallbacks.erase(mCallbacks.begin() + (ID - nCallbackRemoveOffset));
		nCallbackRemoveOffset++;
	}

}
