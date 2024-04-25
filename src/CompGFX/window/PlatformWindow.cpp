#include "PlatformWindow.hpp"
#include <Utility/CppUtility.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <atomic>
#include <unordered_map>

#if defined(_DEBUG)
constexpr bool DebugEnabled = true;
#else
constexpr bool DebugEnabled = false;
#endif

namespace egx {

	static thread_local std::unordered_map<GLFWwindow*, PlatformWindow*> s_Internal_Windows;

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
		if (key == GLFW_KEY_UNKNOWN) return;
		if (action == GLFW_REPEAT) return;
		PlatformWindow* w = _Internal_GetWindow(window);
		if (!w)
			return;
		w->_key_state[key] = action == GLFW_PRESS;
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

	void PlatformWindow::ResetConfigrations()
	{
		glfwInit();
		glfwDefaultWindowHints();
	}

	void PlatformWindow::ConfigureResizable(bool resizable)
	{
		glfwInit();
		glfwWindowHint(GLFW_RESIZABLE, resizable);
	}

	void PlatformWindow::ConfigureVisible(bool visible)
	{
		glfwInit();
		glfwWindowHint(GLFW_VISIBLE, visible);
	}

	void PlatformWindow::ConfigureDecorated(bool decorated)
	{
		glfwInit();
		glfwWindowHint(GLFW_DECORATED, decorated);
	}

	void PlatformWindow::ConfigureFocused(bool focused)
	{
		glfwInit();
		glfwWindowHint(GLFW_FOCUSED, focused);
	}

	void PlatformWindow::ConfigureFloating(bool floating)
	{
		glfwInit();
		glfwWindowHint(GLFW_FLOATING, floating);
	}

	void PlatformWindow::ConfigureMaximized(bool maximized)
	{
		glfwInit();
		glfwWindowHint(GLFW_MAXIMIZED, maximized);
	}

	static void glfwSetWindowCenter(GLFWwindow* window) {
		// Get window position and size
		int window_x, window_y;
		glfwGetWindowPos(window, &window_x, &window_y);

		int window_width, window_height;
		glfwGetWindowSize(window, &window_width, &window_height);

		// Halve the window size and use it to adjust the window position to the center of the window
		window_width /= 2;
		window_height /= 2;

		window_x += window_width;
		window_y += window_height;

		// Get the list of monitors
		int monitors_length;
		GLFWmonitor** monitors = glfwGetMonitors(&monitors_length);

		if (monitors == NULL) {
			// Got no monitors back
			return;
		}

		// Figure out which monitor the window is in
		GLFWmonitor* owner = NULL;
		int owner_x, owner_y, owner_width, owner_height;

		for (int i = 0; i < monitors_length; i++) {
			// Get the monitor position
			int monitor_x, monitor_y;
			glfwGetMonitorPos(monitors[i], &monitor_x, &monitor_y);

			// Get the monitor size from its video mode
			int monitor_width, monitor_height;
			GLFWvidmode* monitor_vidmode = (GLFWvidmode*)glfwGetVideoMode(monitors[i]);

			if (monitor_vidmode == NULL) {
				// Video mode is required for width and height, so skip this monitor
				continue;

			}
			else {
				monitor_width = monitor_vidmode->width;
				monitor_height = monitor_vidmode->height;
			}

			// Set the owner to this monitor if the center of the window is within its bounding box
			if ((window_x > monitor_x && window_x < (monitor_x + monitor_width)) && (window_y > monitor_y && window_y < (monitor_y + monitor_height))) {
				owner = monitors[i];

				owner_x = monitor_x;
				owner_y = monitor_y;

				owner_width = monitor_width;
				owner_height = monitor_height;
			}
		}

		if (owner != NULL) {
			// Set the window position to the center of the owner monitor
			glfwSetWindowPos(window, owner_x + (owner_width / 2) - window_width, owner_y + (owner_height / 2) - window_height);
		}
	}

	PlatformWindow::PlatformWindow(std::string title, int width, int height)
		: m_width(width), m_height(height), nCallbackRemoveOffset(0)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

		if (m_window == nullptr) {
			LOG(ERR, "Failed to create window.");
			throw std::runtime_error("Failed to create window.");
		}

		LOG(INFO, "Window '{0}' create {1}x{2}", title, width, height);

		glfwSetWindowSizeCallback(m_window, _Internal_WindowResizeCallback);
		glfwSetWindowFocusCallback(m_window, _Internal_WindowFocusCallback);
		glfwSetKeyCallback(m_window, _Internal_WindowKeyCallback);
		glfwSetMouseButtonCallback(m_window, _Internal_WindowMouseButtonCallback);
		glfwSetCursorPosCallback(m_window, _Internal_WindowMouseMove);
		if (!glfwRawMouseMotionSupported()) {
			LOG(ERR, "Raw Input is not supported on your device?");
		}
		else {
			//glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}
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

	 int PlatformWindow::GetWidth()
	 {
		 glfwGetWindowSize(m_window, &m_width, &m_height);
		 return m_width;
	 }

	 int PlatformWindow::GetHeight()
	 {
		 glfwGetWindowSize(m_window, &m_width, &m_height);
		 return m_height;
	 }

	 bool PlatformWindow::IsWindowFocus()
	 {
		 return m_focus;
	 }

	 bool PlatformWindow::IsWindowMinimized()
	{
		int w, h;
		glfwGetWindowSize(m_window, &w, &h);
		return !(w && h);
	}

	void PlatformWindow::CenterWindow()
	{
		glfwSetWindowCenter(m_window);
	}

	 int PlatformWindow::RegisterCallback(EventFlagBits events, const std::function<void(const Event& e, void* pUserDefined)>& func, void* pUserDefined) {
		PlatformWindowCallback callback;
		callback.pFunc = func;
		callback.pUserDefined = pUserDefined;
		mCallbacks.push_back(std::make_pair(events, callback));
		return (int)mCallbacks.size() - 1 + nCallbackRemoveOffset;
	}

	 void PlatformWindow::RemoveCallback(int ID)
	{
		// ID is basically the index in vector, however since we remove elements
		// their indices changes theirfore we use this offset to get the correct index
		try {
			mCallbacks.erase(mCallbacks.begin() + (ID - nCallbackRemoveOffset));
			nCallbackRemoveOffset++;
		} catch (...) {}
	}

	bool PlatformWindow::GetKeyState(uint16_t KeyCode)
	{
		assert(KeyCode <= GLFW_KEY_LAST && "Key code is outside of key range. Invalid key code.");
		if (KeyCode <= 255) {
			KeyCode = toupper(KeyCode);
		}
		return _key_state[KeyCode];
	}

	void PlatformWindow::SetFullscreen(bool state)
	{
		if (state) {
			auto monitor = glfwGetPrimaryMonitor();
			auto mode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		}
		else {
			glfwSetWindowMonitor(m_window, NULL, 0, 0, 1920, 1080, 0);
			CenterWindow();
		}
	}

}
