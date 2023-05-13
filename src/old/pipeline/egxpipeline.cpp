#include "egxpipeline.hpp"
#include <cassert>
#include <Utility/CppUtility.hpp>

egx::ref<egx::Pipeline>  egx::Pipeline::FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface)
{
	return ref<Pipeline>(new Pipeline(CoreInterface));
}

egx::Pipeline::~Pipeline()
{
	if (Pipeline_) {
		vkDestroyPipeline(_coreinterface->Device, Pipeline_, nullptr);
	}
}

egx::Pipeline::Pipeline(Pipeline&& move) noexcept :
	_coreinterface(move._coreinterface)
{
	memcpy(this, &move, sizeof(Pipeline));
	memset(&move, 0, sizeof(Pipeline));
}

egx::Pipeline& egx::Pipeline::operator=(Pipeline&& move) noexcept
{
	memcpy(this, &move, sizeof(Pipeline));
	memset(&move, 0, sizeof(Pipeline));
	return *this;
}

VkPipelineColorBlendAttachmentState egx::Pipeline::NormalBlendingPreset()
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

VkPipelineColorBlendAttachmentState egx::Pipeline::AdditiveBlendingPreset()
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

VkPipelineColorBlendAttachmentState egx::Pipeline::SubtractBlendingPreset()
{
	VkPipelineColorBlendAttachmentState state = AdditiveBlendingPreset();
	state.colorBlendOp = VK_BLEND_OP_SUBTRACT;
	state.alphaBlendOp = VK_BLEND_OP_SUBTRACT;
	return state;
}

VkPipelineColorBlendAttachmentState egx::Pipeline::MultiplyBlendingPreset()
{
	return VkPipelineColorBlendAttachmentState();
}

VkPipelineColorBlendAttachmentState egx::Pipeline::DefaultBlendingPreset()
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

VkPipelineColorBlendAttachmentState egx::Pipeline::CombinedAdditiveAlphaBlendingPreset()
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

void  egx::Pipeline::Create(
	const ref<Shader2>& vertex,
	const ref<Shader2>& fragment,
	const ref<Framebuffer>& framebuffer,
	const uint32_t PassId,
	const std::map<uint32_t, VkPipelineColorBlendAttachmentState>& customBlendStates)
{
	_graphics = true;
	if (Pipeline_) {
		vkDestroyPipeline(_coreinterface->Device, Pipeline_, nullptr);
		_Sets.clear();
	}


	const auto& vreflect = vertex->Reflection;
	const auto& freflect = fragment->Reflection;
	Pool = SetPool::FactoryCreate(_coreinterface);
	Layout = PipelineLayout::FactoryCreate(_coreinterface);

	// Create Pipeline Layout
	std::vector<VkPushConstantRange> ranges;
	if (vreflect.Pushconstant.HasValue) {
		auto& pc = vreflect.Pushconstant;
		VkPushConstantRange range{};
		range.offset = pc.Offset;
		range.size = pc.Size;
		range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		ranges.push_back(range);
	}
	if (freflect.Pushconstant.HasValue) {
		auto& pc = freflect.Pushconstant;
		VkPushConstantRange range{};
		range.offset = pc.Offset;
		range.size = pc.Size;
		range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		ranges.push_back(range);
	}

	if (ranges.size() == 2) {
		auto& v = ranges[0];
		auto& f = ranges[1];
		if (v.offset == f.offset && v.size == f.size) {
			ranges.pop_back();
			ranges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		else {
			ranges[0].stageFlags = ranges[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		}
	}

	for (auto& range : ranges) {
		Layout->AddPushconstantRange(range.offset, range.size, range.stageFlags);
	}

	// Create Descriptor Sets
	auto createSets = [&](const ShaderReflection& reflect) {
		for (auto& [setId, bindings] : reflect.SetToManyBindings) {
			if (!_Sets.contains(setId)) {
				auto set = DescriptorSet::FactoryCreate(_coreinterface, setId);
				for (auto& [bindingId, binding] : bindings) {
					if (binding.IsBuffer)
						set->DescribeBuffer(bindingId, binding.Type, VK_SHADER_STAGE_ALL_GRAPHICS);
					else
						set->DescribeImage(bindingId, binding.DescriptorCount, binding.Type, VK_SHADER_STAGE_FRAGMENT_BIT);
				}
				_Sets[setId] = set;
			}
			else {
				auto& set = _Sets[setId];
				for (auto& [bindingId, binding] : bindings) {
					if (binding.IsBuffer)
						set->DescribeBuffer(bindingId, binding.Type, VK_SHADER_STAGE_ALL_GRAPHICS);
					else
						set->DescribeImage(bindingId, binding.DescriptorCount, binding.Type, VK_SHADER_STAGE_FRAGMENT_BIT);
				}
			}
		}
	};
	createSets(vreflect);
	createSets(freflect);
	Pool->AdjustForRequirements(_Sets);
	for (auto& [id, set] : _Sets) {
		set->AllocateSet(Pool);
		Layout->AddSets({ set });
	}

	VkPipelineShaderStageCreateInfo Stages[2]{};
	Stages[0].sType = Stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	Stages[0].pNext = Stages[1].pNext = 0;
	Stages[0].flags = Stages[1].flags = 0;
	Stages[0].pSpecializationInfo = Stages[1].pSpecializationInfo = 0;
	Stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	Stages[0].module = vertex->ShaderModule;
	Stages[0].pName = "main";
	VkSpecializationInfo vertexConstants = vertex->GetSpecializationInfo();
	VkSpecializationInfo fragmentConstants = fragment->GetSpecializationInfo();
	Stages[0].pSpecializationInfo = &vertexConstants;

	Stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	Stages[1].module = fragment->ShaderModule;
	Stages[1].pName = "main";
	Stages[1].pSpecializationInfo = &fragmentConstants;

	VkPipelineInputAssemblyStateCreateInfo InputAssemblyState{};
	InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	InputAssemblyState.pNext = nullptr;
	InputAssemblyState.flags = 0;
	InputAssemblyState.topology = Topology;
	InputAssemblyState.primitiveRestartEnable = VK_FALSE;

	VkPipelineTessellationStateCreateInfo TessellationState{};
	TessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	TessellationState.pNext = nullptr;
	TessellationState.flags = 0;
	TessellationState.patchControlPoints = 1;

	// The viewport and scissor are based on the dimensions of the framebuffer.
	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = float(ViewportWidth == 0 ? framebuffer->Width : ViewportWidth);
	viewport.height = float(ViewportHeight == 0 ? framebuffer->Height : ViewportHeight);
	viewport.minDepth = NearField;
	viewport.maxDepth = FarField;

	VkRect2D scissor{};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = uint32_t(viewport.width);
	scissor.extent.height = uint32_t(viewport.height);

	VkPipelineViewportStateCreateInfo ViewportState{};
	ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	ViewportState.pNext = nullptr;
	ViewportState.flags = 0;
	ViewportState.viewportCount = 1;
	ViewportState.pViewports = &viewport;
	ViewportState.scissorCount = 1;
	ViewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo RasterizationState{};
	RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	RasterizationState.pNext = nullptr;
	RasterizationState.flags = 0;
	RasterizationState.depthClampEnable = VK_FALSE;
	RasterizationState.rasterizerDiscardEnable = VK_FALSE;
	RasterizationState.polygonMode = FillMode;
	RasterizationState.cullMode = CullMode;
	RasterizationState.frontFace = FrontFace;
	RasterizationState.depthBiasEnable = VK_FALSE;
	RasterizationState.depthBiasConstantFactor = 0.0f;
	RasterizationState.depthBiasClamp = 0.0f;
	RasterizationState.depthBiasSlopeFactor = 0.0f;
	RasterizationState.lineWidth = LineWidth;

	VkPipelineMultisampleStateCreateInfo MultisampleState{};
	MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	MultisampleState.pNext = nullptr;
	MultisampleState.flags = 0;
	MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	// TODO: Determine if sample shading is useful and makes a visual difference
	MultisampleState.sampleShadingEnable = VK_FALSE;
	MultisampleState.minSampleShading = 0.0f;
	MultisampleState.pSampleMask = nullptr;
	MultisampleState.alphaToCoverageEnable = VK_FALSE;
	MultisampleState.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo DepthStencilState{};
	DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	DepthStencilState.pNext = nullptr;
	DepthStencilState.flags = 0;
	DepthStencilState.depthTestEnable = DepthEnabled;
	DepthStencilState.depthWriteEnable = DepthWriteEnable;
	DepthStencilState.depthCompareOp = DepthCompare;
	DepthStencilState.depthBoundsTestEnable = VK_FALSE;
	DepthStencilState.stencilTestEnable = VK_FALSE;
	DepthStencilState.front = {};
	DepthStencilState.back = {};
	DepthStencilState.minDepthBounds = 0.0f;
	DepthStencilState.maxDepthBounds = 1.0f;

	std::vector<VkPipelineColorBlendAttachmentState> BlendAttachmentStates;
	{
		const auto& subpass = framebuffer->_subpass[PassId];
		uint32_t offset = framebuffer->_depthattachment.has_value() ? 1 : 0;
		for (uint32_t i = 0; i < subpass.colorAttachmentCount; i++)
		{
			auto& colorAttachment = subpass.pColorAttachments[i];
			uint32_t colorAttachmentId = colorAttachment.attachment - offset;
			if (customBlendStates.contains(colorAttachmentId)) {
				BlendAttachmentStates.push_back(customBlendStates.at(colorAttachmentId));
			}
			else {
				BlendAttachmentStates.push_back(framebuffer->_colorattachements.at(colorAttachmentId).GetBlendState());
			}
		}
	}

	VkPipelineColorBlendStateCreateInfo ColorBlendState{};
	ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	ColorBlendState.pNext = nullptr;
	ColorBlendState.flags = 0;
	ColorBlendState.logicOpEnable = VK_FALSE;
	ColorBlendState.logicOp = VK_LOGIC_OP_MAX_ENUM;
	ColorBlendState.attachmentCount = (uint32_t)BlendAttachmentStates.size();
	ColorBlendState.pAttachments = BlendAttachmentStates.data();
	ColorBlendState.blendConstants[0] = 0.0;
	ColorBlendState.blendConstants[1] = 0.0;
	ColorBlendState.blendConstants[2] = 0.0;
	ColorBlendState.blendConstants[3] = 0.0;

	VkPipelineDynamicStateCreateInfo DynamicState{};
	DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	DynamicState.pNext = nullptr;
	DynamicState.flags = 0;
	DynamicState.dynamicStateCount = 0;
	DynamicState.pDynamicStates = nullptr;

#if 0
	VkPipelineRenderingCreateInfo dynamicCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	dynamicCreateInfo.viewMask = 0;
	dynamicCreateInfo.colorAttachmentCount = (uint32_t)framebuffer->_colorattachements.size();
	std::vector<VkFormat> colorFormat(dynamicCreateInfo.colorAttachmentCount);
	{
		int i = 0;
		for (const auto& [id, attachment] : framebuffer->_colorattachements) {
			colorFormat[i] = attachment.Attachment->Format;
			i++;
		}
	}
	if (colorFormat.size() > 0) {
		dynamicCreateInfo.pColorAttachmentFormats = &colorFormat[0];
	}
	else {
		dynamicCreateInfo.pColorAttachmentFormats = nullptr;
	}
	if (framebuffer->_depthattachment.has_value()) {
		dynamicCreateInfo.depthAttachmentFormat = framebuffer->_depthattachment->Attachment->Format;
		dynamicCreateInfo.stencilAttachmentFormat = framebuffer->_depthattachment->Attachment->Format;
	}
#endif

	VkGraphicsPipelineCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.pNext = nullptr; //&dynamicCreateInfo;
#ifdef _DEBUG
	createInfo.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT | VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR;
#endif
	VkPipelineVertexInputStateCreateInfo VertexInputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	std::vector<VkVertexInputAttributeDescription> VertexAttributes;
	std::vector<VkVertexInputBindingDescription> VertexBinding;
	{
		const auto& vertexShaderInputs = vreflect.IOBindingToManyLocationIn;
		for (auto& [bindingId, input] : vertexShaderInputs) {
			uint32_t offset = 0;
			for (auto& [locationId, io] : input) {
				VkVertexInputAttributeDescription attribute{};
				attribute.binding = io.Binding;
				attribute.format = io.Format;
				attribute.location = io.Location;
				attribute.offset = offset;
				offset += io.Size;
				VertexAttributes.push_back(attribute);
			}
			VkVertexInputBindingDescription inputBinding{};
			inputBinding.binding = bindingId;
			inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			inputBinding.stride = offset;
			VertexBinding.push_back(inputBinding);
		}
		//auto bindingList = vertexDescription.GetBindings();
		//VertexBinding.resize(bindingList.size());
		//int i = 0;
		//for (uint32_t bindingId : bindingList) {
		//	VertexBinding[i].binding = bindingId;
		//	VertexBinding[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		//	VertexBinding[i].stride = vertexDescription.GetStride(bindingId);
		//	i++;
		//}
		//for (auto& e : vertexDescription.Attributes) {
		//	VkVertexInputAttributeDescription attribute{};
		//	attribute.binding = e.BindingId;
		//	attribute.format = e.Format;
		//	attribute.location = e.Location;
		//	attribute.offset = e.Offset;
		//	VertexAttributes.push_back(attribute);
		//}
		VertexInputState.vertexBindingDescriptionCount = (uint32_t)VertexBinding.size();
		VertexInputState.pVertexBindingDescriptions = VertexBinding.data();
		VertexInputState.vertexAttributeDescriptionCount = (uint32_t)VertexAttributes.size();
		VertexInputState.pVertexAttributeDescriptions = VertexAttributes.data();
	}
	createInfo.stageCount = 2;
	createInfo.pStages = Stages;
	createInfo.pVertexInputState = &VertexInputState;
	createInfo.pInputAssemblyState = &InputAssemblyState;
	createInfo.pTessellationState = &TessellationState;
	createInfo.pViewportState = &ViewportState;
	createInfo.pRasterizationState = &RasterizationState;
	createInfo.pMultisampleState = &MultisampleState;
	createInfo.pDepthStencilState = &DepthStencilState;
	createInfo.pColorBlendState = &ColorBlendState;
	createInfo.pDynamicState = &DynamicState;
	createInfo.layout = Layout->GetLayout();
	createInfo.renderPass = framebuffer->GetRenderPass();
	createInfo.subpass = PassId;
	createInfo.basePipelineHandle = 0;
	createInfo.basePipelineIndex = 0;

	vkCreateGraphicsPipelines(_coreinterface->Device, nullptr, 1, &createInfo, 0, &Pipeline_);
}

void egx::Pipeline::Create(const ref<Shader2>& compute)
{
	_graphics = false;
	if (Pipeline_) {
		vkDestroyPipeline(_coreinterface->Device, Pipeline_, nullptr);
		_Sets.clear();
	}

	Layout = PipelineLayout::FactoryCreate(_coreinterface);
	Pool = SetPool::FactoryCreate(_coreinterface);
	const auto& reflection = compute->Reflection;
	if (reflection.Pushconstant.HasValue) {
		Layout->AddPushconstantRange(reflection.Pushconstant.Offset, reflection.Pushconstant.Size, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	auto createSets = [&](const ShaderReflection& reflect) {
		for (auto& [setId, bindings] : reflect.SetToManyBindings) {
			auto set = DescriptorSet::FactoryCreate(_coreinterface, setId);
			for (auto& [bindingId, binding] : bindings) {
				if (binding.IsBuffer)
					set->DescribeBuffer(bindingId, binding.Type, VK_SHADER_STAGE_COMPUTE_BIT);
				else
					set->DescribeImage(bindingId, binding.DescriptorCount, binding.Type, VK_SHADER_STAGE_COMPUTE_BIT);
			}
			_Sets[setId] = std::move(set);
		}
	};
	createSets(reflection);
	Pool->AdjustForRequirements(_Sets);
	for (auto& [id, set] : _Sets) {
		set->AllocateSet(Pool);
		Layout->AddSets({ set });
	}

	VkComputePipelineCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createInfo.stage.pNext = nullptr;
	createInfo.stage.flags = 0;
	createInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	createInfo.stage.module = compute->ShaderModule;
	createInfo.stage.pName = "main";
	VkSpecializationInfo info = compute->GetSpecializationInfo();
	createInfo.stage.pSpecializationInfo = &info;
	createInfo.layout = Layout->GetLayout();
	createInfo.basePipelineHandle = nullptr;
	createInfo.basePipelineIndex = 0;
	vkCreateComputePipelines(_coreinterface->Device, nullptr, 1, &createInfo, nullptr, &Pipeline_);
}
