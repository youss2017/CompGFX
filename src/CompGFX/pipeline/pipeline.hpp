#pragma once
#include "shaders/shader.hpp"
#include <memory/egxbuffer.hpp>
#include <memory/egximage.hpp>
#include "RenderTarget.hpp"
#include <memory>

namespace egx
{

	class PipelineType : public IUniqueHandle
	{
	public:
		virtual inline vk::PipelineBindPoint BindPoint() const = 0;
		virtual inline vk::Pipeline Pipeline() const = 0;
		virtual inline std::map<uint32_t, vk::DescriptorSetLayout> GetDescriptorSetLayouts() const = 0;
		virtual inline ShaderReflection Reflection() const = 0;
		virtual vk::PipelineLayout Layout() const = 0;
	};

	class NullPipelineType : public PipelineType {
	public:
		virtual inline vk::PipelineBindPoint BindPoint() const { return {}; }
		virtual inline vk::Pipeline Pipeline() const { return{}; }
		virtual inline std::map<uint32_t, vk::DescriptorSetLayout> GetDescriptorSetLayouts() const { return{}; }
		virtual inline ShaderReflection Reflection() const { return {}; }
		// This function is used to allow polymorphism
		virtual std::unique_ptr<IUniqueHandle> MakeHandle() const { return nullptr; }
	};

	class ComputePipeline : public PipelineType
	{
	public:
		ComputePipeline() = default;
		ComputePipeline(const DeviceCtx& pCtx, const Shader& computeShader);

		virtual vk::Pipeline Pipeline() const override
		{
			return m_Data->m_Pipeline;
		}

		vk::PipelineLayout Layout() const
		{
			return m_Data->m_Layout;
		}

		virtual inline vk::PipelineBindPoint BindPoint() const override { return vk::PipelineBindPoint::eCompute; }
		virtual inline std::map<uint32_t, vk::DescriptorSetLayout> GetDescriptorSetLayouts() const override { return m_Data->m_SetLayouts; }
		virtual inline ShaderReflection Reflection() const override { return m_Data->m_Reflection; }
		virtual std::unique_ptr<IUniqueHandle> MakeHandle() const override {
			std::unique_ptr<ComputePipeline> handle = std::make_unique<ComputePipeline>();
			handle->m_Data = m_Data;
			return handle;
		}

	private:
		struct DataWrapper
		{
			DeviceCtx m_Ctx;
			ShaderReflection m_Reflection;
			std::map<uint32_t, vk::DescriptorSetLayout> m_SetLayouts;
			vk::PipelineLayout m_Layout = nullptr;
			vk::Pipeline m_Pipeline = nullptr;

			DataWrapper() = default;
			DataWrapper(DataWrapper&) = delete;
			~DataWrapper();
		};

		std::shared_ptr<DataWrapper> m_Data;
	};

	struct PipelineSpecification {
		/// Pipeline properties. You do not have to set viewport width/height
		// the default is to use the framebuffer
		vk::CullModeFlags CullMode = (vk::CullModeFlags)VK_CULL_MODE_NONE;
		vk::FrontFace FrontFace = (vk::FrontFace)VK_FRONT_FACE_COUNTER_CLOCKWISE;
		vk::CompareOp DepthCompare = (vk::CompareOp)VK_COMPARE_OP_ALWAYS;
		vk::PolygonMode FillMode = (vk::PolygonMode)VK_POLYGON_MODE_FILL;
		vk::PrimitiveTopology Topology = (vk::PrimitiveTopology)VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		float NearField = 0.0f;
		float FarField = 1.0f;
		float LineWidth = 1.0f;
		uint32_t ViewportWidth = 0;
		uint32_t ViewportHeight = 0;
		bool DepthEnabled = false;
		bool DepthWriteEnable = true;
		bool DynamicViewport = false;
		bool DynamicScissor = false;

		PipelineSpecification& SetCullMode(vk::CullModeFlags cullMode) { CullMode = cullMode; return *this; }
		PipelineSpecification& SetFrontFace(vk::FrontFace frontFace) { FrontFace = frontFace; return *this; }
		PipelineSpecification& SetDepthCompare(vk::CompareOp depthCompare) { DepthCompare = depthCompare; return *this; }
		PipelineSpecification& SetFillMode(vk::PolygonMode fillMode) { FillMode = fillMode; return *this; }
		PipelineSpecification& SetTopology(vk::PrimitiveTopology topology) { Topology = topology; return *this; }
		PipelineSpecification& SetDepthFields(float nearField, float farField) { NearField = nearField, FarField = farField; return *this; }
		PipelineSpecification& SetLineWidth(float width = 1.0f) { LineWidth = width; return *this; }
		PipelineSpecification& SetViewport(uint32_t width, uint32_t height) { ViewportWidth = width, ViewportHeight = height; return *this; }
		PipelineSpecification& SetDepthEnabled(bool enabled, bool writeEnable) { DepthEnabled = enabled, DepthWriteEnable = writeEnable; return *this; }
		PipelineSpecification& UseDynamicViewport() { DynamicViewport = true; return *this; }
		PipelineSpecification& UseDynamicScissor() { DynamicScissor = true; return *this; }
	};

	class IGraphicsPipeline : public PipelineType, public ICallback
	{
	public:
		IGraphicsPipeline() = default;
		IGraphicsPipeline(const DeviceCtx& pCtx, const Shader& vertex, const Shader& fragment, const IRenderTarget& rt, const PipelineSpecification& specification = {});

		virtual inline vk::PipelineBindPoint BindPoint() const override { return vk::PipelineBindPoint::eGraphics; }
		virtual inline std::map<uint32_t, vk::DescriptorSetLayout> GetDescriptorSetLayouts() const override { return m_Data->m_SetLayouts; }
		virtual inline ShaderReflection Reflection() const override { return m_Data->m_Reflection; }

		IGraphicsPipeline& SetBlendingState(uint32_t colorAttachmentId, const vk::PipelineColorBlendAttachmentState& state)
		{
			if (!m_Data->m_RenderTarget.ContainsAttachment(colorAttachmentId))
			{
				throw std::runtime_error(cpp::Format("Attachment={} not found.", colorAttachmentId));
			}
			m_Data->m_BlendStates[colorAttachmentId] = state;
			return *this;
		}

		virtual vk::Pipeline Pipeline() const override
		{
			return m_Data->m_Pipeline;
		}

		vk::PipelineLayout Layout() const
		{
			return m_Data->m_Layout;
		}

		virtual std::unique_ptr<IUniqueHandle> MakeHandle() const override {
			std::unique_ptr<IGraphicsPipeline> handle = std::make_unique<IGraphicsPipeline>();
			handle->m_Data = m_Data;
			return handle;
		}

		vk::PipelineColorBlendAttachmentState NormalBlendingPreset()
		{
			VkPipelineColorBlendAttachmentState state{};
			state.blendEnable = true;
			state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			state.colorBlendOp = VK_BLEND_OP_ADD;
			state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			state.alphaBlendOp = VK_BLEND_OP_ADD;
			state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			return state;
		}

		vk::PipelineColorBlendAttachmentState AdditiveBlendingPreset()
		{
			VkPipelineColorBlendAttachmentState state{};
			state.blendEnable = true;
			state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
			state.colorBlendOp = VK_BLEND_OP_ADD;
			state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			state.alphaBlendOp = VK_BLEND_OP_ADD;
			state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			return state;
		}

		vk::PipelineColorBlendAttachmentState SubtractBlendingPreset()
		{
			VkPipelineColorBlendAttachmentState state = AdditiveBlendingPreset();
			state.colorBlendOp = VK_BLEND_OP_SUBTRACT;
			state.alphaBlendOp = VK_BLEND_OP_SUBTRACT;
			return state;
		}

		vk::PipelineColorBlendAttachmentState DefaultBlendingPreset()
		{
			VkPipelineColorBlendAttachmentState state{};
			state.blendEnable = false;
			state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			state.colorBlendOp = VK_BLEND_OP_ADD;
			state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			state.alphaBlendOp = VK_BLEND_OP_ADD;
			state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			return state;
		}

		vk::PipelineColorBlendAttachmentState CombinedAdditiveAlphaBlendingPreset()
		{
			VkPipelineColorBlendAttachmentState state{};
			state.blendEnable = true;
			state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			state.colorBlendOp = VK_BLEND_OP_ADD;
			state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			state.alphaBlendOp = VK_BLEND_OP_ADD;
			state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			return state;
		}
		IGraphicsPipeline& SetRenderTarget(const IRenderTarget& renderTarget) { m_Data->m_RenderTarget = renderTarget; return *this; }
		void Invalidate();

		virtual void CallbackProtocol(void* pUserData) override;
	
	public:
		PipelineSpecification Specification;


	private:
		struct DataWrapper
		{
			DeviceCtx m_Ctx;
			ShaderReflection m_Reflection;
			std::map<uint32_t, vk::DescriptorSetLayout> m_SetLayouts;
			vk::PipelineLayout m_Layout = nullptr;
			vk::Pipeline m_Pipeline = nullptr;
			std::vector<vk::Pipeline> m_SwapchainPipelines;
			std::map<uint32_t, vk::PipelineColorBlendAttachmentState> m_BlendStates;
			IRenderTarget m_RenderTarget;
			Shader m_Vertex;
			Shader m_Fragment;

			void Reinvalidate();

			DataWrapper() = default;
			DataWrapper(DataWrapper&) = delete;
			~DataWrapper();
		};

		std::shared_ptr<DataWrapper> m_Data;
	};

}