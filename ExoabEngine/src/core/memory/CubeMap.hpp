#pragma once
#include <EngineBase.h>
#include <backend/backend_base.h>
#include <memory/vulkan_memory_allocator.hpp>
#include <memory/Texture2.hpp>
#include <string>

ITexture2 CubeMap_Create(const std::string& path, VkFormat format);
