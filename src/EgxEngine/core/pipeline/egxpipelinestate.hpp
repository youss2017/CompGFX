#pragma once
#include "egxpipeline.hpp"
#include <cassert>

namespace egx {

	class PipelineState {
	public:
		EGX_API PipelineState(ref<VulkanCoreInterface>& CoreInterface, uint32_t Width = 0, uint32_t Height = 0);
		EGX_API PipelineState(PipelineState&) = delete;
		EGX_API PipelineState(PipelineState&& move);
		EGX_API PipelineState& operator=(PipelineState& move);

		EGX_API void AddSet(ref<egxshaderset>& set);
		EGX_API void AddPushconstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags stages);

		EGX_API void CreateColorAttachment(
			uint32_t ColorAttachmentID,
			VkFormat Format,
			VkClearValue ClearValue,
			VkImageUsageFlags CustomUsageFlags,
			VkImageLayout ImageLayout,
			VkAttachmentLoadOp LoadOp,
			VkAttachmentStoreOp StoreOp,
			VkPipelineColorBlendAttachmentState* pBlendState = nullptr);

		EGX_API void CreateDepthAttachment(
			VkFormat Format,
			VkClearValue ClearValue,
			VkImageUsageFlags CustomUsageFlags,
			VkImageLayout ImageLayout,
			VkAttachmentLoadOp LoadOp,
			VkAttachmentStoreOp StoreOp);

		// Make sure to have at least 1 attachement (color or depth) before calling this function
		EGX_API void CreateRasterPipeline(
			const egxshader& vertex,
			const egxshader& fragment,
			const egxvertexdescription& vertexDescription);

		void EGX_API CreateCompuePipeline(const egxshader& compute);

		/// <summary>
		/// Make sure the pipeline is not used.
		/// </summary>
		/// <param name="compute">The reloaded compute shader, must have the same layout</param>
		void EGX_API ReloadComputePipeline(const egxshader& compute);

		/// <summary>
		/// Make sure the pipeline is not used.
		/// </summary>
		/// <param name="compute">The reloaded compute shader, must have the same layout</param>
		void EGX_API ReloadRasterPipeline(
			const egxshader& vertex,
			const egxshader& fragment,
			const egxvertexdescription& vertexDescription);

		inline void SetDynamicOffset(uint32_t SetId, uint32_t BindingId, uint32_t Offset) {
			Layout->SetDynamicOffset(SetId, BindingId, Offset);
		}

		inline void Bind(VkCommandBuffer cmd) {
			if (Framebuffer.IsValidRef()) {
				auto& RenderingInfo = Framebuffer->GetRenderingInfo();
				vkCmdBeginRendering(cmd, &RenderingInfo);
				Pipeline->bind(cmd);
				Layout->bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS);
			}
			else {
				Pipeline->bind(cmd);
				Layout->bind(cmd, VK_PIPELINE_BIND_POINT_COMPUTE);
			}
		}

		inline void EndGraphics(VkCommandBuffer cmd) { assert(Framebuffer.IsValidRef()); vkCmdEndRendering(cmd); }

	public:
		ref<PipelineLayout> Layout;
		ref<egxframebuffer> Framebuffer;
		ref<Pipeline> Pipeline;
		uint32_t Width;
		uint32_t Height;
	private:
		ref<VulkanCoreInterface> _core;
		bool _isgraphics = true;
	};

}
