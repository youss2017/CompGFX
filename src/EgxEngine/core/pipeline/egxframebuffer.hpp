#pragma once
#include "../egxcommon.hpp"
#include "../memory/egxmemory.hpp"
#include <vector>
#include <optional>

namespace egx {

	namespace _internal {
		struct egfxframebuffer_attachment {
			ref<Image> Attachment;
			VkImageView View{};
			VkRenderingAttachmentInfo AttachmentInfo{};
			std::optional<VkPipelineColorBlendAttachmentState> BlendState;

			inline VkPipelineColorBlendAttachmentState GetBlendState() const {
				if (BlendState.has_value()) return *BlendState;
				// Thanks to https://vulkan-tutorial.com
				VkPipelineColorBlendAttachmentState colorBlendAttachment{};
				// ColorWriteMask is critical, if not set correctly no/partial data will be written the framebuffer
				colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				colorBlendAttachment.blendEnable = VK_FALSE;
				colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
				colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
				colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
				colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
				colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
				colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
				return colorBlendAttachment;
			}

		};
	}

	class egxframebuffer {

	public:
		static ref<egxframebuffer> EGX_API FactoryCreate(ref<VulkanCoreInterface>& CoreInterface, uint32_t Width, uint32_t Height);

		egxframebuffer(egxframebuffer& cp) = delete;
		egxframebuffer(egxframebuffer&& move) = delete;
		egxframebuffer& operator=(egxframebuffer&& move) = delete;

		void EGX_API CreateColorAttachment(
			uint32_t ColorAttachmentID,
			VkFormat Format,
			VkClearValue ClearValue,
			VkImageUsageFlags CustomUsageFlags,
			VkImageLayout ImageLayout,
			VkAttachmentLoadOp LoadOp,
			VkAttachmentStoreOp StoreOp,
			VkPipelineColorBlendAttachmentState* pBlendState = nullptr);

		void EGX_API CreateDepthAttachment(
			VkFormat Format,
			VkClearValue ClearValue,
			VkImageUsageFlags CustomUsageFlags,
			VkImageLayout ImageLayout,
			VkAttachmentLoadOp LoadOp,
			VkAttachmentStoreOp StoreOp);

		inline VkRenderingAttachmentInfo EGX_API GetColorAttachmentInfo(uint32_t ColorAttachmentID, VkImageLayout layout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp) const
		{
			assert(_colorattachements.find(ColorAttachmentID) != _colorattachements.end());
			const auto& attachment = _colorattachements.at(ColorAttachmentID);
			return attachment.AttachmentInfo;
		}

		inline VkRenderingAttachmentInfo EGX_API GetDepthAttachmentInfo(VkImageLayout layout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp) const {
			assert(_depthattachment.has_value());
			return _depthattachment->AttachmentInfo;
		}

		inline ref<Image> GetColorAttachment(uint32_t ColorAttachmentID) {
			return _colorattachements[ColorAttachmentID].Attachment;
		}

		inline ref<Image> GetDepthAttachment() {
			if (_depthattachment.has_value())
				return _depthattachment->Attachment;
			return { (Image*)nullptr };
		}
		
		/*
		void EGX_API start();
		void EGX_API end();
		*/

		VkRenderingInfoKHR& GetRenderingInfo() {
			if (_renderinfo.has_value()) return *_renderinfo;
			_dummycolorattachmentlist.clear();
			for (auto& [Id, attach] : _colorattachements) {
				_dummycolorattachmentlist.push_back(attach.AttachmentInfo);
			}
			_dummycolorattachmentlist.shrink_to_fit();
			VkRenderingInfoKHR renderInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO_KHR };
			renderInfo.renderArea = {{}, {Width, Height}};
			renderInfo.layerCount = (uint32_t)_dummycolorattachmentlist.size();
			renderInfo.viewMask = 0;
			renderInfo.colorAttachmentCount = (uint32_t)_dummycolorattachmentlist.size();
			renderInfo.pColorAttachments = _dummycolorattachmentlist.data();
			renderInfo.pDepthAttachment = _depthattachment.has_value() ? &_depthattachment->AttachmentInfo : nullptr;
			renderInfo.pStencilAttachment = nullptr;
			_renderinfo = renderInfo;
			return *_renderinfo;
		}

	protected:
		egxframebuffer(ref<VulkanCoreInterface>& CoreInterface, uint32_t width, uint32_t height) :
			_coreinterface(CoreInterface), Width(width), Height(height)
		{}

	public:
		const uint32_t Width;
		const uint32_t Height;

	protected:
		friend class Pipeline;
		ref<VulkanCoreInterface> _coreinterface;
		std::map<uint32_t, _internal::egfxframebuffer_attachment> _colorattachements;
		std::optional<_internal::egfxframebuffer_attachment> _depthattachment;
		std::optional<VkRenderingInfoKHR> _renderinfo;
		std::vector<VkRenderingAttachmentInfoKHR> _dummycolorattachmentlist;
		// VkRenderPass _renderpass = nullptr;
	};

}
