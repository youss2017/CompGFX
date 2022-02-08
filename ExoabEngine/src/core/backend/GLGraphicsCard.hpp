#pragma once
#include "../../window/PlatformWindow.hpp"
#include <string>
#include <vector>
#include "backend_base.h"
#include <glad/glad.h>

namespace gl
{
	struct _tagGlContext
	{
		char ApiType; // 0 for vulkan, 1 for opengl
		PlatformWindow* window;
		int32_t ContextMajorVersion;
		int32_t ContextMinorVersion;
		uint32_t MaxSupportedMSAASamples;

		// features
		bool SupportBufferStorage = false; // glBufferStorage
		bool SupportTextureStorage = false; // glTextureStorage...
		bool SupportMinSampleShading = false; // glMinSampleShading
		bool SupportShadingLanguage420Pack = false; // GL_ARB_shading_language_420pack
		bool SupportUniformBufferObject = false; // GL_ARB_uniform_buffer_object

	} typedef *GLContext;

	// set major version to 0 for GL compatiability mode.
    GLContext Gfx_CreateContext(PlatformWindow* window, int MajorVersion, int MinorVersion, bool EnableDebug);

	// All OpenGL Extensions TYPICALLY starts with GL_ and not ARB_
	bool Gfx_CheckExtensionSupport(const char* extension);

	// not really necessary for OpenGL, but its best practice.
	void Gfx_DestroyContext(GLContext context);

    void Gfx_SetVSyncState(int SwapInterval);
	// switch context!
	void Gfx_MakeCurrentContext(GLContext context);
	// When switching context to another thread, detach the context from the original thread
	void Gfx_DetachContext(GLContext context);
	void Gfx_Present(GLContext context);

}

