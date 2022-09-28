#include "egxsampler.hpp"

egx::Sampler::Sampler(const ref<VulkanCoreInterface>& CoreInterface) :
	_coreinterface(CoreInterface)
{}

egx::Sampler::~Sampler()
{
	if (_sampler)
		vkDestroySampler(_coreinterface->Device, _sampler, nullptr);
}

egx::Sampler::Sampler(Sampler&& move) noexcept
{
	memcpy(this, &move, sizeof(Sampler));
	memset(&move, 0, sizeof(Sampler));
}

egx::Sampler& egx::Sampler::operator=(Sampler& move) noexcept
{
	memcpy(this, &move, sizeof(Sampler));
	memset(&move, 0, sizeof(Sampler));
	return *this;
}

VkSampler& egx::Sampler::GetSampler()
{
	if (_sampler) return _sampler;
	VkSamplerCreateInfo createInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	createInfo.magFilter = magFilter;
	createInfo.minFilter = minFilter;
	createInfo.mipmapMode = mipmapMode;
	createInfo.addressModeU = addressModeU;
	createInfo.addressModeV = addressModeV;
	createInfo.addressModeW = addressModeW;
	createInfo.mipLodBias = mipLodBias;
	createInfo.anisotropyEnable = anisotropyEnable;
	createInfo.maxAnisotropy = maxAnisotropy;
	createInfo.compareEnable = compareEnable;
	createInfo.compareOp = compareOp;
	createInfo.minLod = minLod;
	createInfo.maxLod = maxLod;
	createInfo.borderColor = borderColor;
	createInfo.unnormalizedCoordinates = unnormalizedCoordinates;
	vkCreateSampler(_coreinterface->Device, &createInfo, nullptr, &_sampler);
	return _sampler;
}

float egx::Sampler::GetMaxAnisotropyLevel(const ref<VulkanCoreInterface>& CoreInterface)
{
	return CoreInterface->PhysicalDevice.Properties.limits.maxSamplerAnisotropy;
}