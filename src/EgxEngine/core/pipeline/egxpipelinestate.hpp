#pragma once
#include "egxpipeline.hpp"
#include <cassert>

namespace egx {

	class PipelineState {
	public:
		EGX_API PipelineState(const ref<VulkanCoreInterface>& CoreInterface, uint32_t Width = 0, uint32_t Height = 0);
		EGX_API PipelineState(const ref<VulkanCoreInterface>& CoreInterface, const ref<Framebuffer>& Framebuffer);
		EGX_API PipelineState(PipelineState&& move);
		EGX_API PipelineState& operator=(PipelineState& move);

		EGX_API PipelineState(const PipelineState& copy) = default;

		EGX_API void AddSet(ref<egxshaderset>& set);
		EGX_API void AddPushconstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags stages);

		// Make sure to have at least 1 attachement (color or depth) before calling this function
		EGX_API void CreateRasterPipeline(
			const egxshader& vertex,
			const egxshader& fragment,
			const uint32_t PassId,
			const egxvertexdescription& vertexDescription);

		void EGX_API CreateCompuePipeline(const egxshader& compute);

		inline void SetDynamicOffset(uint32_t SetId, uint32_t BindingId, uint32_t Offset) {
			Layout->SetDynamicOffset(SetId, BindingId, Offset);
		}

		inline void Bind(VkCommandBuffer cmd) {
			if (Framebuffer.IsValidRef()) {
				std::vector<VkClearValue> clearValues;
				clearValues.reserve(15);
				if (Framebuffer->_depthattachment.has_value() && Framebuffer->_depthattachment->Description.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
					clearValues.push_back(Framebuffer->_depthattachment->ClearValue);
				}
				for (auto& [id, attachment] : Framebuffer->_colorattachements) {
					if (attachment.Description.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
						clearValues.push_back(attachment.ClearValue);
					}
				}
				VkRenderPassBeginInfo beginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
				beginInfo.renderPass = Framebuffer->GetRenderPass();
				beginInfo.framebuffer = Framebuffer->GetFramebuffer();
				beginInfo.renderArea = { {0,0}, {Width, Height} };
				beginInfo.clearValueCount = clearValues.size();
				beginInfo.pClearValues = clearValues.data();
				vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
				Pipeline->bind(cmd);
				Layout->bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS);
			}
			else {
				Pipeline->bind(cmd);
				Layout->bind(cmd, VK_PIPELINE_BIND_POINT_COMPUTE);
			}
		}

		inline void EndGraphics(VkCommandBuffer cmd) { assert(Framebuffer.IsValidRef()); vkCmdEndRenderPass(cmd); }

	public:
		ref<PipelineLayout> Layout;
		ref<Framebuffer> Framebuffer;
		ref<Pipeline> Pipeline;
		uint32_t Width;
		uint32_t Height;
	private:
		ref<VulkanCoreInterface> _core;
	};

}
