#pragma once
#include "../egxcommon.hpp"
#include "../memory/egxmemory.hpp"
#include <vector>
#include <optional>
#include <cstdint>
#include <unordered_map>

namespace egx {

	namespace _internal {
		struct egfxframebuffer_attachment {
			ref<Image> Attachment;
			VkImageView View{};
			VkAttachmentDescription Description{};
			VkClearValue ClearValue{};
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

	class Framebuffer {

	public:
		static ref<Framebuffer > EGX_API FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface, uint32_t Width, uint32_t Height);

		Framebuffer(Framebuffer& cp) = delete;
		Framebuffer(Framebuffer&& move) = delete;
		Framebuffer& operator=(Framebuffer&& move) = delete;
		EGX_API ~Framebuffer();

		/// <summary>
		/// Creates color attachment for the framebuffer (aka pass)
		/// </summary>
		/// <param name="ColorAttachmentID">An integer id used to reference this Color Attachment</param>
		/// <param name="Format">The format of the attachment</param>
		/// <param name="ClearValue">If loadOp is clear, then when the pass is started, this value is used to clear the attachment</param>
		/// <param name="loadOp">Determines whether to load, clear, or don't care about the attachment's content</param>
		/// <param name="storeOp">Determine's where to store/don't care about the attachment's content</param>
		/// <param name="InitialLayout">The inital layout before the pass has started</param>
		/// <param name="PassLayout">The layout used during the pass</param>
		/// <param name="FinalLayout">The final layout after the pass.</param>
		/// <param name="CustomUsageFlags">Additional usage flags such as GENERAL, SHADER_READ_ONLY_OPTIMAL for shader use</param>
		/// <param name="pBlendState">For Alpha blending</param>
		/// <returns></returns>
		void EGX_API CreateColorAttachment(
			uint32_t ColorAttachmentId,
			VkFormat Format,
			VkClearValue ClearValue,
			VkAttachmentLoadOp LoadOp,
			VkAttachmentStoreOp StoreOp,
			VkImageLayout InitialLayout,
			VkImageLayout FinalLayout,
			VkImageUsageFlags CustomUsageFlags = 0,
			VkPipelineColorBlendAttachmentState* pBlendState = nullptr
		);

		// Same as CreateColorAttachment
		void EGX_API CreateDepthAttachment(
			VkFormat Format,
			VkClearValue ClearValue,
			VkAttachmentLoadOp LoadOp,
			VkAttachmentStoreOp StoreOp,
			VkImageLayout InitialLayout,
			VkImageLayout FinalLayout,
			VkImageUsageFlags CustomUsageFlags = 0);


		// std::pair<uint32_t, VkImageLayout>
		// The uint32_t is the ColorAttachmentId
		// The VkImageLayout is the image layout to use during the pass aka (VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		// Note: subpasses are in order of creation
		// Returns PassId
		EGX_API uint32_t CreateSubpass(
			const std::vector<std::pair<uint32_t, VkImageLayout>>& ColorAttachmentIds, 
			std::optional<VkImageLayout> DepthAttachment = {},
			const std::vector<std::pair<uint32_t, VkImageLayout>>& InputAttachments = {});

		inline ref<Image> GetColorAttachment(uint32_t ColorAttachmentID) {
			return _colorattachements[ColorAttachmentID].Attachment;
		}

		inline ref<Image> GetDepthAttachment() {
			if (_depthattachment.has_value())
				return _depthattachment->Attachment;
			return { (Image*)nullptr };
		}

		// Once you call GetFramebuffer you no longer can create any pass or any attachment
		// On the first call to this function the framebuffer is created.
		VkFramebuffer& GetFramebuffer() noexcept {
			if (_framebuffer) return _framebuffer;
			InternalCreate();
			return _framebuffer;
		}

		// Once you call GetRenderPass you no longer can create any pass or any attachment
		// On the first call to this function the render pass is created.
		VkRenderPass& GetRenderPass() noexcept {
			if (_renderpass) return _renderpass;
			InternalCreate();
			return _renderpass;
		}
		
	protected:
		Framebuffer(const ref<VulkanCoreInterface>& CoreInterface, uint32_t width, uint32_t height) :
			_coreinterface(CoreInterface), Width(width), Height(height)
		{
			_attachment_ref.reserve(1000);
		}

	private:
		EGX_API void InternalCreate();

	public:
		const uint32_t Width;
		const uint32_t Height;

	protected:
		friend class Pipeline;
		friend class PipelineState;
		ref<VulkanCoreInterface> _coreinterface;
		std::unordered_map<uint32_t, _internal::egfxframebuffer_attachment> _colorattachements;
		std::optional<_internal::egfxframebuffer_attachment> _depthattachment;
		// std::optional<VkRenderingInfoKHR> _renderinfo;
		std::vector<VkRenderingAttachmentInfoKHR> _dummycolorattachmentlist;
		std::vector<VkAttachmentReference> _attachment_ref;
		std::vector<VkSubpassDescription> _subpass;
		VkFramebuffer _framebuffer = nullptr;
		VkRenderPass _renderpass = nullptr;
	};

}
