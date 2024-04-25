#pragma once
#include <core/egx.hpp>
#include "Event.hpp"
#include <functional>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <string>

namespace egx {

	struct PlatformWindowCallback {
		std::function<void(const Event& e, void* pUserDefined)> pFunc;
		void* pUserDefined;
	};

	class PlatformWindow {

	public:

		static void ResetConfigrations();
		static void ConfigureResizable(bool resizable);
		static void ConfigureVisible(bool visible);
		static void ConfigureDecorated(bool decorated);
		static void ConfigureFocused(bool focused);
		static void ConfigureFloating(bool floating);
		static void ConfigureMaximized(bool maximized);

		PlatformWindow() = default;
		PlatformWindow(std::string title, int width, int height);
		~PlatformWindow();

		static void Poll();

		inline GLFWwindow* GetWindow() const { return m_window; }
		bool ShouldClose();
		int GetWidth();
		int GetHeight();
		bool IsWindowFocus();
		bool IsWindowMinimized();

		void CenterWindow();

		// This function returns ID (index) of the callback which can be used later to remove the callback
		int RegisterCallback(EventFlagBits events, const std::function<void(const Event& e, void* pUserDefined)>& func, void* pUserDefined = nullptr);
		void RemoveCallback(int ID);
		glm::vec2 GetWindowSize() { return glm::vec2(m_width, m_height); }

		/// @brief Checks the key state (pressed or not)
		/// @param KeyCode you can enter key as ascii code e.g. 'A' or 'a' or you can enter GLFW VK codes ex: GLFW_KEY_UP 
		/// @return Returns whether the key is pressed or not
		bool GetKeyState(uint16_t KeyCode);

		void SetFullscreen(bool state);

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
