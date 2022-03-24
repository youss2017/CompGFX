#pragma once
#include <stdint.h>
#include <memory/Buffer2.hpp>
#include <memory/Texture2.hpp>

enum BindingType {
	BINDING_TYPE_BUFFER,
	BINDING_TYPE_IMAGE
};

enum BindingFlags {
	BINDING_CREATE_BUFFERS,
	BINDING_CREATE_BUFFER_POINTER,
	BINDING_PREINITALIZED,
	BINDING_TEXTURE
};

struct BindingDescription {
	uint32_t mBindingID;
	BindingFlags mFlags;
	BindingType mType;
	union {
		IBuffer2 mBuffers;
		struct {
			ITexture2 mTexture;
			VkSampler mSampler;
		};
	};
};

