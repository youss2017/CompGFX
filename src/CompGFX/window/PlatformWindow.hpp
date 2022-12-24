#pragma once
#include "../core/egxcommon.hpp"
#include <string>
#include "Event.hpp"
#include <functional>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>

namespace egx {

	struct PlatformWindowCallback {
		std::function<void(const Event& e, void* pUserDefined)> pFunc;
		void* pUserDefined;
	};

	class PlatformWindow {

	public:
		EGX_API PlatformWindow(std::string title, int width, int height);
		EGX_API ~PlatformWindow();

		EGX_API static void Poll();

		inline GLFWwindow* GetWindow() { return m_window; }
		EGX_API bool ShouldClose();
		inline int GetWidth() { return m_width; }
		inline int GetHeight() { return m_height; }
		inline bool IsWindowFocus() { return m_focus; }
		EGX_API bool IsWindowMinimized();

		// This function returns ID (index) of the callback which can be used later to remove the callback
		EGX_API int RegisterCallback(EventFlagBits events, const std::function<void(const Event& e, void* pUserDefined)>& func, void* pUserDefined);
		EGX_API void RemoveCallback(int ID);
		glm::vec2 GetWindowSize() { return glm::vec2(m_width, m_height); }

		/// @brief Checks the key state (pressed or not)
		/// @param KeyCode you can enter key as ascii code e.g. 'A' or 'a' or you can eneter GLFW VK codes ex: GLFW_KEY_UP 
		/// @return Returns whether the key is pressed or not
		EGX_API bool GetKeyState(uint16_t KeyCode);

	private:
		int m_width;
		int m_height;
		GLFWwindow* m_window;
		bool m_focus = true;
		int nCallbackRemoveOffset;
		std::vector<std::pair<EventFlagBits, PlatformWindowCallback>> mCallbacks;
		bool _key_state[GLFW_KEY_LAST + 1]{};
	private:
		friend void _Internal_WindowResizeCallback(GLFWwindow* window, int width, int height);
		friend void _Internal_WindowKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		friend void _Internal_WindowFocusCallback(GLFWwindow* window, int focused);
		friend void _Internal_WindowMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		friend void _Internal_WindowMouseMove(GLFWwindow* window, double xpos, double ypos);
	};

}
