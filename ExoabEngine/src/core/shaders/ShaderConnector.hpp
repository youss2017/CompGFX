#pragma once
#include <stdint.h>
#include <memory/Buffer2.hpp>
#include <memory/Texture2.hpp>

enum BindingFlagBits : unsigned int {
	BINDING_FLAG_CPU_VISIBLE			= 0x00001,
	BINDING_FLAG_CREATE_BUFFERS_POINTER	= 0x00010,
	BINDING_FLAG_REQUIRE_COHERENT		= 0x00100,
	BINDING_FLAG_BUFFER_USAGE_INDIRECT  = 0x01000,
	BINDING_FLAG_USING_ONLY_FIRST_MIP	= 0x10000,
};

typedef uint32_t BindingFlags;

/*
	For textures (VkImageLayout)
	BINDING_TYPE_COMBINED_TEXTURE_SAMPLER is assumed to be used as VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	BINDING_TYPE_STORAGE_IMAGE is assumed to be used as VK_IMAGE_LAYOUT_GENERAL
	Textures must be initalized!
*/
struct BindingDescription {
	uint32_t mBindingID;
	BindingFlags mFlags;
	VkDescriptorType mType;
	VkShaderStageFlags mStages;
	uint32_t mBufferSize;
	IBuffer2 mBuffer;
	std::vector<ITexture2> mTextures;
	VkSampler mSampler;
	// Custom VkImageLayout, <id in mTextures, VkImageLayout>
	std::map<uint32_t, VkImageLayout> mCustomImageLayouts;
	bool mSharedResources;
};

struct DescriptorSet {
	uint32_t mSetID;
	std::map<uint32_t, BindingDescription> mBindings;
	VkDescriptorSet mSets[gFrameOverlapCount];
	VkDescriptorSetLayout mSetLayout;
	VkDescriptorPool mPool;

	VkBuffer GetBuffer(uint32_t bindingID, uint32_t FrameIndex) {
		assert(mBindings.find(bindingID) != mBindings.end());
		auto& binding = mBindings[bindingID];
		return binding.mBuffer->mBuffers[FrameIndex];
	}

	IBuffer2 GetBuffer2(uint32_t bindingID) {
		assert(mBindings.find(bindingID) != mBindings.end());
		auto& binding = mBindings[bindingID];
		return binding.mBuffer;
	}

	const VkDescriptorSet& operator[](uint32_t FrameIndex) {
		return mSets[FrameIndex];
	}

};

struct DescriptorSetSharedResources {
	DescriptorSet* mPartner;
	std::vector<uint32_t> mSharedBindings;
};

void ShaderConnector_CalculateDescriptorPool(uint32_t bindingsCount, BindingDescription* pBindings, std::vector<VkDescriptorPoolSize>& poolSizes);
DescriptorSet ShaderConnector_CreateSet(uint32_t setID, VkDescriptorPool pool, uint32_t bindingsCount, BindingDescription* pBindings, uint32_t sharedResourceCount, DescriptorSetSharedResources* pSharedResources);
VkPipelineLayout ShaderConnector_CreatePipelineLayout(uint32_t descriptorSetCount, DescriptorSet* pSets, std::vector<VkPushConstantRange> ranges);
void ShaderConnector_DestroySet(const DescriptorSet& set);
