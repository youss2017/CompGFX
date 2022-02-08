#include "GLGraphicsCard.hpp"
#include "../../utils/common.hpp"
#include "../../utils/Logger.hpp"
#include "../memory/Buffers.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cassert>

namespace gl
{
	void APIENTRY GlDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
	{
		// ignore non-significant error/warning codes
		// if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

		std::string result;

		result += "---------------";
		result += "\n";
		result += "Debug message (" + std::to_string(id) + "): ";
		result += message;
		result += "\n";

		switch (source)
		{
		case GL_DEBUG_SOURCE_API:
			result += "Source: API";
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			result += "Source: Window System";
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			result += "Source: Shader Compiler";
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			result += "Source: Third Party";
			break;
		case GL_DEBUG_SOURCE_APPLICATION:
			result += "Source: Application";
			break;
		case GL_DEBUG_SOURCE_OTHER:
			result += "Source: Other";
			break;
		}
		result += "\n";

		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:
			result += "Type: Error";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			result += "Type: Deprecated Behaviour";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			result += "Type: Undefined Behaviour";
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			result += "Type: Portability";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			result += "Type: Performance";
			break;
		case GL_DEBUG_TYPE_MARKER:
			result += "Type: Marker";
			break;
		case GL_DEBUG_TYPE_PUSH_GROUP:
			result += "Type: Push Group";
			break;
		case GL_DEBUG_TYPE_POP_GROUP:
			result += "Type: Pop Group";
			break;
		case GL_DEBUG_TYPE_OTHER:
			result += "Type: Other";
			break;
		}
		result += "\n";

		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:
			result += "Severity: high";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			result += "Severity: medium";
			break;
		case GL_DEBUG_SEVERITY_LOW:
			result += "Severity: low";
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			result += "Severity: notification";
			break;
		}
		result += "\n";
		if (log_error(result.c_str()))
			Utils::Break();
	}

	static void Gfx_Internal_DebugState(bool DebugState)
	{
		if (DebugState)
		{
			// Make sure to disable this, since this is not supported by OpenGL 3.3 (needs 4.2 or 4.3) or check for its extension
			if (Gfx_CheckExtensionSupport("GL_ARB_debug_output") || Gfx_CheckExtensionSupport("GL_KHR_debug"))
			{
				glEnable(GL_DEBUG_OUTPUT);
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
				if (glDebugMessageCallbackARB)
					glDebugMessageCallbackARB(GlDebugProc, 0);
				else if (glDebugMessageCallbackKHR)
					glDebugMessageCallbackKHR(GlDebugProc, 0);
				else
					glDebugMessageCallback(GlDebugProc, 0);
				if (glDebugMessageControlARB)
					glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
				else if (glDebugMessageControlKHR)
					glDebugMessageControlKHR(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
			}
			else
			{
				logwarning("Could not enable debugging for OpenGL because the extension is not supported!");
			}
		}
		else
		{
			glDisable(GL_DEBUG_OUTPUT);
			glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		}
	}

	GLContext Gfx_CreateContext(PlatformWindow *window, int MajorVersion, int MinorVersion, bool EnableDebug)
	{
		assert(window);
		GLContext context = new _tagGlContext;
		context->ApiType = 1;
		context->window = window;
		if (EnableDebug)
			Gfx_Internal_DebugState(true);
		else
			Gfx_Internal_DebugState(false);
		char info[200];
		info[0] = 0;
		info[0] = 0;
		const char *version = (const char *)glGetString(GL_VERSION);
		const char *renderer = (const char *)glGetString(GL_RENDERER);
		sprintf(info, "%s --- %s", renderer, version);
		loginfo(info);
		glGetIntegerv(GL_MAX_SAMPLES, (GLint *)&context->MaxSupportedMSAASamples);
		return context;
	}

	void Gfx_DestroyContext(GLContext context)
	{
		assert(context);
		Gfx_DetachContext(context);
		free(context);
	}

	void Gfx_SetVSyncState(int SwapInterval)
	{
		glfwSwapInterval(SwapInterval);
	}

	void Gfx_MakeCurrentContext(GLContext context)
	{
		assert(context);
		Gfx_DetachContext(context);
		logwarning("Switching OpenGL Context");
		glfwMakeContextCurrent(context->window->GetWindow());
	}

	void Gfx_DetachContext(GLContext context)
	{
		logwarning("Detached OpenGL Context");
		glfwMakeContextCurrent(nullptr);
	}

	// "GL_ARB_debug_output"
	bool Gfx_CheckExtensionSupport(const char *extension)
	{
		int NumberOfExtensions;
		glGetIntegerv(GL_NUM_EXTENSIONS, &NumberOfExtensions);
		for (int i = 0; i < NumberOfExtensions; i++)
		{
			const char *ccc = (const char *)glGetStringi(GL_EXTENSIONS, i);
			if (strcmp(ccc, extension) == 0)
			{
				return true;
			}
		}
		return false;
	}

	void Gfx_Present(GLContext context)
	{
		glfwSwapBuffers(context->window->GetWindow());
	}

}

//SwapBuffers((HDC)context->DeviceContext);

/*
		if (!wglSwapIntervalEXT)
		{
			wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
			if(!wglSwapIntervalEXT) {
				log_warning("[Win32 API] Cannot change swap interval since WGL_EXT_swap_control is not supported.");
			}
		}
		wglSwapIntervalEXT(SwapInterval);
*/


/*
		HWND hwnd = (HWND)window->window_handle;
		HDC DeviceContext = GetDC(hwnd);
		PIXELFORMATDESCRIPTOR pfd;
		ZeroMemory(&pfd, sizeof(pfd));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 32;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int iPfd = ChoosePixelFormat(DeviceContext, &pfd);
		DescribePixelFormat(DeviceContext, iPfd, sizeof(pfd), &pfd);
		SetPixelFormat(DeviceContext, iPfd, &pfd);
		HGLRC compRenderContext = wglCreateContext(DeviceContext);
		HGLRC renderContext = 0;
		wglMakeCurrent(DeviceContext, compRenderContext);
		static bool LOADED_GL = false;

		if (!LOADED_GL)
		{
			if (EnableDebug)
			{
				auto MyLoader = [](const char *name) throw()->void *
				{
					void *func = wglGetProcAddress(name);
					if (func == NULL)
					{
						func = (void *)GetProcAddress(GL_LIB, name);
					}
					// if (!func) {
					//	//char buffer[150];
					//	//buffer[0] = 0;
					//	//strcat(buffer, "Could not load OpenGL Function ");
					//	//strcat(buffer, " ---> ");
					//	//strcat(buffer, name);
					//	//log_warning(buffer);
					// }
					return func;
				};
				gladLoadGLLoader((GLADloadproc)MyLoader);
			}
			else
				gladLoadGL();
			LOADED_GL = true;
		}
		GLContext context = (GLContext)malloc(sizeof(_tagGlContext));
		assert(context);
		context->DeviceContext = DeviceContext;
		context->ApiType = 1;
		context->IsDebugMode = EnableDebug;
		if (MajorVersion >= 3)
		{
			PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
			if (!wglCreateContextAttribsARB)
			{
				log_alert("Graphics Card does not support core profile! Using compatability profile instead--this may cause is issues because the card may be too old to support 3.3.");
				// driver does not support extensions, probably too old for it too.
				context->ContextMajorVersion = 0;
				context->ContextMinorVersion = 0;
				context->IsCoreProfile = false;
				context->RenderContext = compRenderContext;
				if (EnableDebug)
					Gfx_Internal_DebugState(true);
				else
					Gfx_Internal_DebugState(false);
				char info[200];
				info[0] = 0;
				strcat(info, "OpenGL WGL Compatability Context ");
				sprintf(info, "%s 0x%p --- ", info, compRenderContext);
				strcat(info, "WGL Core Context ");
				sprintf(info, "%s 0x%p", info, renderContext);
				log_info(info);
				info[0] = 0;
				const char *renderer = (const char *)glGetString(GL_RENDERER);
				const char *version = (const char *)glGetString(GL_VERSION);
				sprintf(info, "%s --- %s", renderer, version);
				return context;
			}
			int attributes[7];

			attributes[0] = WGL_CONTEXT_MAJOR_VERSION_ARB;
			attributes[1] = MajorVersion;
			attributes[2] = WGL_CONTEXT_MINOR_VERSION_ARB;
			attributes[3] = MinorVersion;
			if (EnableDebug)
			{
				attributes[4] = WGL_CONTEXT_FLAGS_ARB;
			}
			else
			{
				attributes[4] = WGL_CONTEXT_PROFILE_MASK_ARB;
			}
			attributes[5] = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
			attributes[6] = 0;

			renderContext = wglCreateContextAttribsARB(DeviceContext, NULL, attributes);
			wglMakeCurrent(DeviceContext, 0);
			wglDeleteContext(compRenderContext);
			wglMakeCurrent(DeviceContext, renderContext);
			context->ContextMajorVersion = MajorVersion;
			context->ContextMinorVersion = MinorVersion;
			context->IsCoreProfile = true;
		}
*/