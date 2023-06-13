#pragma once
#include "egxbuffer.hpp"
#include <imgui/imgui.h>
#include <map>
#include <glm/vec2.hpp>

namespace egx
{

	class Image2D
	{
	public:
		Image2D() = default;
		Image2D(const DeviceCtx& pCtx, int width, int height, vk::Format format, int mipLevels, vk::ImageUsageFlags usage, vk::ImageLayout initalLayout, bool streaming);
		Image2D(const DeviceCtx &pCtx, glm::ivec2 resolution, vk::Format format, int mipLevels, vk::ImageUsageFlags usage, vk::ImageLayout initalLayout, bool streaming) :
			Image2D(pCtx, resolution.x, resolution.y, format, mipLevels, usage, initalLayout, streaming)
		{}
		static Image2D CreateFromHandle(const DeviceCtx &pCtx, vk::Image handle, int width, int height, vk::Format format, int mipLevels, vk::ImageUsageFlags usage, vk::ImageLayout initalLayout);

		void SetPixel(int mipLevel, int x, int y, const void *pixelData);
		void GetPixel(int mipLevel, int x, int y);

		void SetImageData(int mipLevel, int xOffset, int yOffset, int width, int height, const void *pData);
		void SetImageData(int mipLevel, const void *pData);

		void Read(int mipLevel, int xOffset, int yOffset, int width, int height, void *pOutBuffer);
		void Read(int mipLevel, void *pOutBuffer);

		void GenerateMipmaps();
		void SetLayout(vk::ImageLayout layout);

		vk::ImageView CreateView(int id, int mipLevel = 0, int mipCount = VK_REMAINING_MIP_LEVELS, vk::ComponentMapping RGBASwizzle = {});
		vk::ImageView GetView(int id) const;
		bool ContainsView(int id) const {
			return m_Data->m_Views.contains(id); 
		}

		vk::Image GetHandle() const;
		ImTextureID GetImGuiTextureID(vk::Sampler sampler, uint32_t viewId = 0);

	public:
		int Width;
		int Height;
		vk::Format Format;
		bool StreamingMode;
		vk::ImageUsageFlags Usage;
		vk::ImageLayout CurrentLayout;

	private:
		struct DataWrapper
		{
			DeviceCtx m_Ctx;
			vk::Image m_Image = nullptr;
			std::unique_ptr<egx::Buffer> m_StageBuffer;
			std::map<int, vk::ImageView> m_Views;
			VmaAllocation m_Allocation = nullptr;
			ImTextureID m_TextureID = nullptr;

			DataWrapper() = default;

			DataWrapper(DataWrapper &) = delete;
			DataWrapper(DataWrapper &&) = delete;

			~DataWrapper();
		};

		std::shared_ptr<DataWrapper> m_Data;
		int m_MipLevels;
		int m_TexelBytes;
	};

}