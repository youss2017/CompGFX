#pragma once
#include "../egxcommon.hpp"

namespace egx {

	class Sampler {
	public:
        EGX_API Sampler(const ref<VulkanCoreInterface>& CoreInterface);
        EGX_API ~Sampler();
        Sampler(Sampler&) = delete;
        EGX_API Sampler(Sampler&&) noexcept;
        EGX_API Sampler& operator=(Sampler&) noexcept;

        // You can modify all properties of the Sampler before calling GetSampler()
        // Once GetSampler is called the Sampler is created.
        EGX_API VkSampler& GetSampler();

        static float GetMaxAnisotropyLevel(const ref<VulkanCoreInterface>& CoreInterface);

	public:
        VkFilter                magFilter = VK_FILTER_LINEAR;
        VkFilter                minFilter = VK_FILTER_LINEAR;
        VkSamplerMipmapMode     mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        VkSamplerAddressMode    addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode    addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode    addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        float                   mipLodBias = 0.0;
        VkBool32                anisotropyEnable = VK_FALSE;
        float                   maxAnisotropy = 0.0;
        VkBool32                compareEnable = VK_FALSE;
        VkCompareOp             compareOp = VK_COMPARE_OP_LESS;
        float                   minLod = 0.0;
        float                   maxLod = 1000.0;
        VkBorderColor           borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        VkBool32                unnormalizedCoordinates = VK_FALSE;

	private:
        ref<VulkanCoreInterface> _coreinterface;
        VkSampler _sampler = nullptr;
	};

}
