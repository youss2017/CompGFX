#pragma once
#include "../egxcommon.hpp"
#include "../shaders/egxshader2.hpp"
#include "../shaders/egxshaderset.hpp"
#include "../memory/egxref.hpp"
#include "egxframebuffer.hpp"
#include <set>

namespace egx {

	class Pipeline {
	public:
		static ref<Pipeline> EGX_API FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface);
		EGX_API ~Pipeline();
		EGX_API Pipeline(Pipeline& copy) = delete;
		EGX_API Pipeline(Pipeline&& move) noexcept;
		EGX_API Pipeline& operator=(Pipeline&& move) noexcept;

		/// <summary>
		/// Also known as Alpha Blending
		/// </summary>
		/// <returns></returns>
		EGX_API static VkPipelineColorBlendAttachmentState NormalBlendingPreset();
		EGX_API static VkPipelineColorBlendAttachmentState AdditiveBlendingPreset();
		EGX_API static VkPipelineColorBlendAttachmentState SubtractBlendingPreset();
		EGX_API static VkPipelineColorBlendAttachmentState MultiplyBlendingPreset();
		/// <summary>
		/// No Blending
		/// </summary>
		/// <returns></returns>
		EGX_API static VkPipelineColorBlendAttachmentState DefaultBlendingPreset();
		/// <summary>
		/// For more info see https://www.youtube.com/watch?v=AxopC4yW4uY
		/// </summary>
		/// <returns></returns>
		EGX_API static VkPipelineColorBlendAttachmentState CombinedAdditiveAlphaBlendingPreset();

		/// <summary>
		/// PassId is determined from Framebuffer::CreatePass()
		/// </summary>
		/// <param name="layout"></param>
		/// <param name="vertex"></param>
		/// <param name="fragment"></param>
		/// <param name="framebuffer"></param>
		/// <param name="vertexDescription"></param>
		/// <returns></returns>
		void EGX_API Create(
			const ref<Shader2>& vertex,
			const ref<Shader2>& fragment,
			const ref<Framebuffer>& framebuffer,
			const uint32_t PassId,
			const std::map<uint32_t, VkPipelineColorBlendAttachmentState>& customBlendStates = {});

		void EGX_API Create(const ref<Shader2>& compute);

		inline void Bind(VkCommandBuffer cmd) const {
			vkCmdBindPipeline(cmd, _graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline_);
			Layout->Bind(cmd, _graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE);
		}

		inline void PushConstants(VkCommandBuffer cmd, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pData) {
			vkCmdPushConstants(cmd, Layout->GetLayout(), stageFlags, offset, size, pData);
		}

		inline VkPipeline operator()() const { return Pipeline_; }
		inline VkPipeline operator*() const { return Pipeline_; }

		inline void SetBuffer(uint32_t setId, uint32_t bindingId, const ref<Buffer>& buffer, uint32_t offset = 0, uint32_t structSize = 0) {
			_Sets.at(setId)->SetBuffer(bindingId, buffer, structSize, offset);
		}

		inline void SetSampledImage(uint32_t setId, uint32_t bindingId, const egx::ref<Sampler>& sampler, const egx::ref<Image>& image, VkImageLayout imageLayout, uint32_t viewId) {
			_Sets.at(setId)->SetImage(bindingId, { image }, { sampler }, { imageLayout }, { viewId });
		}

		inline void SetStorageImage(uint32_t setId, uint32_t bindingId, const egx::ref<Image>& image, VkImageLayout imageLayout, uint32_t viewId) {
			_Sets.at(setId)->SetImage(bindingId, { image }, {}, { imageLayout }, { viewId });
		}

		// For imageLayouts, samplers, and viewIds if they only contain one element, then the first element is used for all images
		// otherwise each element in the vector is used per image in order
		inline void SetSampledImages(uint32_t setId, uint32_t bindingId, const std::vector<egx::ref<Sampler>>& samplers, 
			const std::vector<egx::ref<Image>>& images, const std::vector<VkImageLayout>& imageLayouts, const std::vector<uint32_t>& viewIds) {
			_Sets.at(setId)->SetImage(bindingId, images, samplers, imageLayouts, viewIds);
		}

		// For imageLayouts and viewIds if they only contain one element, then the first element is used for all images
		// otherwise each element in the vector is used per image in order
		inline void SetStorageImages(uint32_t setId, uint32_t bindingId, const std::vector<egx::ref<Image>>& images, const std::vector<VkImageLayout>& imageLayouts, const std::vector<uint32_t>& viewIds) {
			_Sets.at(setId)->SetImage(bindingId, images, {}, imageLayouts, viewIds);
		}

		inline void SetInputAttachment(uint32_t setId, uint32_t bindingId, const egx::ref<egx::Image>& image, VkImageLayout imageLayout, uint32_t viewId)
		{
			throw std::runtime_error("Not Implemented.");
			_Sets.at(setId)->SetImage(bindingId, { image }, {}, { imageLayout }, { viewId }, true);
		}

	protected:
		Pipeline(const ref<VulkanCoreInterface>& coreinterface) :
			_coreinterface(coreinterface) {}

	public:
		/// Pipeline properties you do not have to set viewport width/height
		// the default is to use the framebuffer
		VkCullModeFlags CullMode = VK_CULL_MODE_NONE;
		VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		VkCompareOp DepthCompare = VK_COMPARE_OP_ALWAYS;
		VkPolygonMode FillMode = VK_POLYGON_MODE_FILL;
		VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		float NearField = 0.0f;
		float FarField = 1.0f;
		float LineWidth = 1.0f;
		uint32_t ViewportWidth = 0;
		uint32_t ViewportHeight = 0;
		VkPipeline Pipeline_ = nullptr;
		ref<SetPool> Pool;
		ref<PipelineLayout> Layout;
		bool DepthEnabled = false;
		bool DepthWriteEnable = true;

	protected:
		std::map<uint32_t, ref<DescriptorSet>> _Sets;
		ref<VulkanCoreInterface> _coreinterface;
		bool _graphics = false;

	};

}