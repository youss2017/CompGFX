#pragma once
#include "../../utils/Profiling.hpp"
#include "../../utils/defines.h"
#include <stdint.h>

typedef void* APIHandle;
typedef void *GraphicsContext, *GraphicsSwapchain, *OpaqueIdentifier;

struct _FrameInformation {
    unsigned int m_FrameCount;
    unsigned int m_LastFrameIndex;
    unsigned int m_FrameIndex;
    bool m_Initalized;
};

#define ToVKContext(x) ((vk::VkContext)x)
#define ToGLContext(x) ((gl::GLContext)x)
#define GetVKDevice(x) ((ToVKContext(x))->defaultDevice)
