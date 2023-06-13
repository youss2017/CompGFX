#include "FeatureValidation.hpp"
#include "Utility/CppUtility.hpp"
#include <vulkan/vulkan.hpp>
#include <stdio.h>
#include <string.h>

namespace egx {
	static void ConvertStructureTypeToString(VkStructureType type, char* particle_buffer) {
		if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_WORKGROUP_MEMORY_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_WORKGROUP_MEMORY_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_MEMORY_REPORT_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_MEMORY_REPORT_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_2_PLANE_444_FORMATS_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_2_PLANE_444_FORMATS_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_ARM) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_ARM"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RGBA10X6_FORMATS_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RGBA10X6_FORMATS_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_VALVE) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_VALVE"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_FEATURES_HUAWEI) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_FEATURES_HUAWEI"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_RDMA_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_RDMA_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BORDER_COLOR_SWIZZLE_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BORDER_COLOR_SWIZZLE_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINEAR_COLOR_ATTACHMENT_FEATURES_NV) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINEAR_COLOR_ATTACHMENT_FEATURES_NV"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR"); }
		else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR) { strcat(particle_buffer, "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR"); }
		else {
			char tempBuffer[150]{};
			sprintf(tempBuffer, "(VK_STRUCTURE_TYPE_UNKNOWN, could not convert VkStructureType to string. %x)", type);
			strcat(particle_buffer, tempBuffer);
		}
	}

	static void* GetCardFeature(VkStructureType type, VkPhysicalDeviceFeatures2 features) {
		void* pNext = features.pNext;
		while (pNext) {
			if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES) {
				VkPhysicalDeviceShaderDrawParametersFeatures nf = *(VkPhysicalDeviceShaderDrawParametersFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES) {
				VkPhysicalDeviceVulkan11Features nf = *(VkPhysicalDeviceVulkan11Features*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES) {
				VkPhysicalDeviceVulkan12Features nf = *(VkPhysicalDeviceVulkan12Features*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES) {
				VkPhysicalDevice8BitStorageFeatures nf = *(VkPhysicalDevice8BitStorageFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES) {
				VkPhysicalDevice16BitStorageFeatures nf = *(VkPhysicalDevice16BitStorageFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES) {
				VkPhysicalDeviceDynamicRenderingFeatures nf = *(VkPhysicalDeviceDynamicRenderingFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES) {
				VkPhysicalDeviceShaderAtomicInt64Features nf = *(VkPhysicalDeviceShaderAtomicInt64Features*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES) {
				VkPhysicalDeviceShaderFloat16Int8Features nf = *(VkPhysicalDeviceShaderFloat16Int8Features*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES) {
				VkPhysicalDeviceDescriptorIndexingFeatures nf = *(VkPhysicalDeviceDescriptorIndexingFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES) {
				VkPhysicalDeviceScalarBlockLayoutFeatures nf = *(VkPhysicalDeviceScalarBlockLayoutFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES) {
				VkPhysicalDeviceVulkanMemoryModelFeatures nf = *(VkPhysicalDeviceVulkanMemoryModelFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES) {
				VkPhysicalDeviceImagelessFramebufferFeatures nf = *(VkPhysicalDeviceImagelessFramebufferFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES) {
				VkPhysicalDeviceUniformBufferStandardLayoutFeatures nf = *(VkPhysicalDeviceUniformBufferStandardLayoutFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES) {
				VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures nf = *(VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES) {
				VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures nf = *(VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES) {
				VkPhysicalDeviceHostQueryResetFeatures nf = *(VkPhysicalDeviceHostQueryResetFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES) {
				VkPhysicalDeviceTimelineSemaphoreFeatures nf = *(VkPhysicalDeviceTimelineSemaphoreFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES) {
				VkPhysicalDeviceBufferDeviceAddressFeatures nf = *(VkPhysicalDeviceBufferDeviceAddressFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES) {
				VkPhysicalDeviceVulkan13Features nf = *(VkPhysicalDeviceVulkan13Features*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES) {
				VkPhysicalDeviceShaderTerminateInvocationFeatures nf = *(VkPhysicalDeviceShaderTerminateInvocationFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES) {
				VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures nf = *(VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES) {
				VkPhysicalDevicePrivateDataFeatures nf = *(VkPhysicalDevicePrivateDataFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES) {
				VkPhysicalDevicePipelineCreationCacheControlFeatures nf = *(VkPhysicalDevicePipelineCreationCacheControlFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES) {
				VkPhysicalDeviceSynchronization2Features nf = *(VkPhysicalDeviceSynchronization2Features*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_WORKGROUP_MEMORY_FEATURES) {
				VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures nf = *(VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES) {
				VkPhysicalDeviceImageRobustnessFeatures nf = *(VkPhysicalDeviceImageRobustnessFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES) {
				VkPhysicalDeviceSubgroupSizeControlFeatures nf = *(VkPhysicalDeviceSubgroupSizeControlFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES) {
				VkPhysicalDeviceInlineUniformBlockFeatures nf = *(VkPhysicalDeviceInlineUniformBlockFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES) {
				VkPhysicalDeviceTextureCompressionASTCHDRFeatures nf = *(VkPhysicalDeviceTextureCompressionASTCHDRFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES) {
				VkPhysicalDeviceShaderIntegerDotProductFeatures nf = *(VkPhysicalDeviceShaderIntegerDotProductFeatures*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES) {
				VkPhysicalDeviceMaintenance4Features nf = *(VkPhysicalDeviceMaintenance4Features*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR) {
				VkPhysicalDevicePerformanceQueryFeaturesKHR nf = *(VkPhysicalDevicePerformanceQueryFeaturesKHR*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR) {
				VkPhysicalDeviceShaderClockFeaturesKHR nf = *(VkPhysicalDeviceShaderClockFeaturesKHR*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES_EXT) {
				VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR nf = *(VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR) {
				VkPhysicalDeviceFragmentShadingRateFeaturesKHR nf = *(VkPhysicalDeviceFragmentShadingRateFeaturesKHR*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR) {
				VkPhysicalDevicePresentWaitFeaturesKHR nf = *(VkPhysicalDevicePresentWaitFeaturesKHR*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR) {
				VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR nf = *(VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR) {
				VkPhysicalDevicePresentIdFeaturesKHR nf = *(VkPhysicalDevicePresentIdFeaturesKHR*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR) {
				VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR nf = *(VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR) {
				VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR nf = *(VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT) {
				VkPhysicalDeviceTransformFeedbackFeaturesEXT nf = *(VkPhysicalDeviceTransformFeedbackFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV) {
				VkPhysicalDeviceCornerSampledImageFeaturesNV nf = *(VkPhysicalDeviceCornerSampledImageFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT) {
				VkPhysicalDeviceConditionalRenderingFeaturesEXT nf = *(VkPhysicalDeviceConditionalRenderingFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT) {
				VkPhysicalDeviceDepthClipEnableFeaturesEXT nf = *(VkPhysicalDeviceDepthClipEnableFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT) {
				VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT nf = *(VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV) {
				VkPhysicalDeviceShadingRateImageFeaturesNV nf = *(VkPhysicalDeviceShadingRateImageFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT) {
				VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT nf = *(VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV) {
				VkPhysicalDeviceComputeShaderDerivativesFeaturesNV nf = *(VkPhysicalDeviceComputeShaderDerivativesFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV) {
				VkPhysicalDeviceMeshShaderFeaturesNV nf = *(VkPhysicalDeviceMeshShaderFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV) {
				VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV nf = *(VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV) {
				VkPhysicalDeviceShaderImageFootprintFeaturesNV nf = *(VkPhysicalDeviceShaderImageFootprintFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV) {
				VkPhysicalDeviceExclusiveScissorFeaturesNV nf = *(VkPhysicalDeviceExclusiveScissorFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL) {
				VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL nf = *(VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT) {
				VkPhysicalDeviceFragmentDensityMapFeaturesEXT nf = *(VkPhysicalDeviceFragmentDensityMapFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD) {
				VkPhysicalDeviceCoherentMemoryFeaturesAMD nf = *(VkPhysicalDeviceCoherentMemoryFeaturesAMD*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT) {
				VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT nf = *(VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT) {
				VkPhysicalDeviceMemoryPriorityFeaturesEXT nf = *(VkPhysicalDeviceMemoryPriorityFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV) {
				VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV nf = *(VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES) {
				VkPhysicalDeviceBufferDeviceAddressFeaturesEXT nf = *(VkPhysicalDeviceBufferDeviceAddressFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT) {
				VkValidationFeaturesEXT nf = *(VkValidationFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV) {
				VkPhysicalDeviceCooperativeMatrixFeaturesNV nf = *(VkPhysicalDeviceCooperativeMatrixFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV) {
				VkPhysicalDeviceCoverageReductionModeFeaturesNV nf = *(VkPhysicalDeviceCoverageReductionModeFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT) {
				VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT nf = *(VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT) {
				VkPhysicalDeviceYcbcrImageArraysFeaturesEXT nf = *(VkPhysicalDeviceYcbcrImageArraysFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT) {
				VkPhysicalDeviceLineRasterizationFeaturesEXT nf = *(VkPhysicalDeviceLineRasterizationFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT) {
				VkPhysicalDeviceShaderAtomicFloatFeaturesEXT nf = *(VkPhysicalDeviceShaderAtomicFloatFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT) {
				VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT nf = *(VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV) {
				VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV nf = *(VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV) {
				VkPhysicalDeviceInheritedViewportScissorFeaturesNV nf = *(VkPhysicalDeviceInheritedViewportScissorFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT) {
				VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT nf = *(VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_MEMORY_REPORT_FEATURES_EXT) {
				VkPhysicalDeviceDeviceMemoryReportFeaturesEXT nf = *(VkPhysicalDeviceDeviceMemoryReportFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT) {
				VkPhysicalDeviceRobustness2FeaturesEXT nf = *(VkPhysicalDeviceRobustness2FeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT) {
				VkPhysicalDeviceCustomBorderColorFeaturesEXT nf = *(VkPhysicalDeviceCustomBorderColorFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_FEATURES_NV) {
				VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV nf = *(VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV) {
				VkPhysicalDeviceRayTracingMotionBlurFeaturesNV nf = *(VkPhysicalDeviceRayTracingMotionBlurFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_2_PLANE_444_FORMATS_FEATURES_EXT) {
				VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT nf = *(VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT) {
				VkPhysicalDeviceFragmentDensityMap2FeaturesEXT nf = *(VkPhysicalDeviceFragmentDensityMap2FeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT) {
				VkPhysicalDevice4444FormatsFeaturesEXT nf = *(VkPhysicalDevice4444FormatsFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_ARM) {
				VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesARM nf = *(VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesARM*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RGBA10X6_FORMATS_FEATURES_EXT) {
				VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT nf = *(VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_VALVE) {
				VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE nf = *(VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT) {
				VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT nf = *(VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT) {
				VkPhysicalDeviceDepthClipControlFeaturesEXT nf = *(VkPhysicalDeviceDepthClipControlFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT) {
				VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT nf = *(VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_FEATURES_HUAWEI) {
				VkPhysicalDeviceSubpassShadingFeaturesHUAWEI nf = *(VkPhysicalDeviceSubpassShadingFeaturesHUAWEI*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_RDMA_FEATURES_NV) {
				VkPhysicalDeviceExternalMemoryRDMAFeaturesNV nf = *(VkPhysicalDeviceExternalMemoryRDMAFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT) {
				VkPhysicalDeviceExtendedDynamicState2FeaturesEXT nf = *(VkPhysicalDeviceExtendedDynamicState2FeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT) {
				VkPhysicalDeviceColorWriteEnableFeaturesEXT nf = *(VkPhysicalDeviceColorWriteEnableFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT) {
				VkPhysicalDeviceImageViewMinLodFeaturesEXT nf = *(VkPhysicalDeviceImageViewMinLodFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES) {
				VkPhysicalDeviceMultiDrawFeaturesEXT nf = *(VkPhysicalDeviceMultiDrawFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BORDER_COLOR_SWIZZLE_FEATURES_EXT) {
				VkPhysicalDeviceBorderColorSwizzleFeaturesEXT nf = *(VkPhysicalDeviceBorderColorSwizzleFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT) {
				VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT nf = *(VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINEAR_COLOR_ATTACHMENT_FEATURES_NV) {
				VkPhysicalDeviceLinearColorAttachmentFeaturesNV nf = *(VkPhysicalDeviceLinearColorAttachmentFeaturesNV*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR) {
				VkPhysicalDeviceAccelerationStructureFeaturesKHR nf = *(VkPhysicalDeviceAccelerationStructureFeaturesKHR*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR) {
				VkPhysicalDeviceRayTracingPipelineFeaturesKHR nf = *(VkPhysicalDeviceRayTracingPipelineFeaturesKHR*)pNext;
				return pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR) {
				VkPhysicalDeviceRayQueryFeaturesKHR nf = *(VkPhysicalDeviceRayQueryFeaturesKHR*)pNext;
				return pNext;
			}
		}
		LOG(ERR, "Could not validate feature, missing");
		return nullptr;
	}

	VkBool32 ValidateFeatures(VkPhysicalDevice device, VkPhysicalDeviceFeatures2 requiredFeatures)
	{

		// 1) Meets required specifications
		VkPhysicalDeviceFeatures2 features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
		vkGetPhysicalDeviceFeatures2(device, &features);

		VkBool32 MeetSpecification = VK_TRUE;
		if (requiredFeatures.features.robustBufferAccess == VK_TRUE) MeetSpecification &= features.features.robustBufferAccess;
		if (requiredFeatures.features.fullDrawIndexUint32 == VK_TRUE) MeetSpecification &= features.features.fullDrawIndexUint32;
		if (requiredFeatures.features.imageCubeArray == VK_TRUE) MeetSpecification &= features.features.imageCubeArray;
		if (requiredFeatures.features.independentBlend == VK_TRUE) MeetSpecification &= features.features.independentBlend;
		if (requiredFeatures.features.geometryShader == VK_TRUE) MeetSpecification &= features.features.geometryShader;
		if (requiredFeatures.features.tessellationShader == VK_TRUE) MeetSpecification &= features.features.tessellationShader;
		if (requiredFeatures.features.sampleRateShading == VK_TRUE) MeetSpecification &= features.features.sampleRateShading;
		if (requiredFeatures.features.dualSrcBlend == VK_TRUE) MeetSpecification &= features.features.dualSrcBlend;
		if (requiredFeatures.features.logicOp == VK_TRUE) MeetSpecification &= features.features.logicOp;
		if (requiredFeatures.features.multiDrawIndirect == VK_TRUE) MeetSpecification &= features.features.multiDrawIndirect;
		if (requiredFeatures.features.drawIndirectFirstInstance == VK_TRUE) MeetSpecification &= features.features.drawIndirectFirstInstance;
		if (requiredFeatures.features.depthClamp == VK_TRUE) MeetSpecification &= features.features.depthClamp;
		if (requiredFeatures.features.depthBiasClamp == VK_TRUE) MeetSpecification &= features.features.depthBiasClamp;
		if (requiredFeatures.features.fillModeNonSolid == VK_TRUE) MeetSpecification &= features.features.fillModeNonSolid;
		if (requiredFeatures.features.depthBounds == VK_TRUE) MeetSpecification &= features.features.depthBounds;
		if (requiredFeatures.features.wideLines == VK_TRUE) MeetSpecification &= features.features.wideLines;
		if (requiredFeatures.features.largePoints == VK_TRUE) MeetSpecification &= features.features.largePoints;
		if (requiredFeatures.features.alphaToOne == VK_TRUE) MeetSpecification &= features.features.alphaToOne;
		if (requiredFeatures.features.multiViewport == VK_TRUE) MeetSpecification &= features.features.multiViewport;
		if (requiredFeatures.features.samplerAnisotropy == VK_TRUE) MeetSpecification &= features.features.samplerAnisotropy;
		if (requiredFeatures.features.textureCompressionETC2 == VK_TRUE) MeetSpecification &= features.features.textureCompressionETC2;
		if (requiredFeatures.features.textureCompressionASTC_LDR == VK_TRUE) MeetSpecification &= features.features.textureCompressionASTC_LDR;
		if (requiredFeatures.features.textureCompressionBC == VK_TRUE) MeetSpecification &= features.features.textureCompressionBC;
		if (requiredFeatures.features.occlusionQueryPrecise == VK_TRUE) MeetSpecification &= features.features.occlusionQueryPrecise;
		if (requiredFeatures.features.pipelineStatisticsQuery == VK_TRUE) MeetSpecification &= features.features.pipelineStatisticsQuery;
		if (requiredFeatures.features.vertexPipelineStoresAndAtomics == VK_TRUE) MeetSpecification &= features.features.vertexPipelineStoresAndAtomics;
		if (requiredFeatures.features.fragmentStoresAndAtomics == VK_TRUE) MeetSpecification &= features.features.fragmentStoresAndAtomics;
		if (requiredFeatures.features.shaderTessellationAndGeometryPointSize == VK_TRUE) MeetSpecification &= features.features.shaderTessellationAndGeometryPointSize;
		if (requiredFeatures.features.shaderImageGatherExtended == VK_TRUE) MeetSpecification &= features.features.shaderImageGatherExtended;
		if (requiredFeatures.features.shaderStorageImageExtendedFormats == VK_TRUE) MeetSpecification &= features.features.shaderStorageImageExtendedFormats;
		if (requiredFeatures.features.shaderStorageImageMultisample == VK_TRUE) MeetSpecification &= features.features.shaderStorageImageMultisample;
		if (requiredFeatures.features.shaderStorageImageReadWithoutFormat == VK_TRUE) MeetSpecification &= features.features.shaderStorageImageReadWithoutFormat;
		if (requiredFeatures.features.shaderStorageImageWriteWithoutFormat == VK_TRUE) MeetSpecification &= features.features.shaderStorageImageWriteWithoutFormat;
		if (requiredFeatures.features.shaderUniformBufferArrayDynamicIndexing == VK_TRUE) MeetSpecification &= features.features.shaderUniformBufferArrayDynamicIndexing;
		if (requiredFeatures.features.shaderSampledImageArrayDynamicIndexing == VK_TRUE) MeetSpecification &= features.features.shaderSampledImageArrayDynamicIndexing;
		if (requiredFeatures.features.shaderStorageBufferArrayDynamicIndexing == VK_TRUE) MeetSpecification &= features.features.shaderStorageBufferArrayDynamicIndexing;
		if (requiredFeatures.features.shaderStorageImageArrayDynamicIndexing == VK_TRUE) MeetSpecification &= features.features.shaderStorageImageArrayDynamicIndexing;
		if (requiredFeatures.features.shaderClipDistance == VK_TRUE) MeetSpecification &= features.features.shaderClipDistance;
		if (requiredFeatures.features.shaderCullDistance == VK_TRUE) MeetSpecification &= features.features.shaderCullDistance;
		if (requiredFeatures.features.shaderFloat64 == VK_TRUE) MeetSpecification &= features.features.shaderFloat64;
		if (requiredFeatures.features.shaderInt64 == VK_TRUE) MeetSpecification &= features.features.shaderInt64;
		if (requiredFeatures.features.shaderInt16 == VK_TRUE) MeetSpecification &= features.features.shaderInt16;
		if (requiredFeatures.features.shaderResourceResidency == VK_TRUE) MeetSpecification &= features.features.shaderResourceResidency;
		if (requiredFeatures.features.shaderResourceMinLod == VK_TRUE) MeetSpecification &= features.features.shaderResourceMinLod;
		if (requiredFeatures.features.sparseBinding == VK_TRUE) MeetSpecification &= features.features.sparseBinding;
		if (requiredFeatures.features.sparseResidencyBuffer == VK_TRUE) MeetSpecification &= features.features.sparseResidencyBuffer;
		if (requiredFeatures.features.sparseResidencyImage2D == VK_TRUE) MeetSpecification &= features.features.sparseResidencyImage2D;
		if (requiredFeatures.features.sparseResidencyImage3D == VK_TRUE) MeetSpecification &= features.features.sparseResidencyImage3D;
		if (requiredFeatures.features.sparseResidency2Samples == VK_TRUE) MeetSpecification &= features.features.sparseResidency2Samples;
		if (requiredFeatures.features.sparseResidency4Samples == VK_TRUE) MeetSpecification &= features.features.sparseResidency4Samples;
		if (requiredFeatures.features.sparseResidency8Samples == VK_TRUE) MeetSpecification &= features.features.sparseResidency8Samples;
		if (requiredFeatures.features.sparseResidency16Samples == VK_TRUE) MeetSpecification &= features.features.sparseResidency16Samples;
		if (requiredFeatures.features.sparseResidencyAliased == VK_TRUE) MeetSpecification &= features.features.sparseResidencyAliased;
		if (requiredFeatures.features.variableMultisampleRate == VK_TRUE) MeetSpecification &= features.features.variableMultisampleRate;
		if (requiredFeatures.features.inheritedQueries == VK_TRUE) MeetSpecification &= features.features.inheritedQueries;

		return MeetSpecification;

		void* pNext = requiredFeatures.pNext;

#define NF_ERROR(type) { MeetSpecification = false;\
						 char NF_ERROR_LOG[256]{};\
						 char VK_TYPE_STRING[150]{};\
						 ConvertStructureTypeToString(type, VK_TYPE_STRING);\
						 sprintf(NF_ERROR_LOG, "Enabled features from '%s' are not supported by graphics card.", VK_TYPE_STRING);\
						 LOG(ERR, NF_ERROR_LOG);\
						}

#define Acquire(structTypedef) structTypedef nf = *(structTypedef*)pNext; structTypedef src = *(structTypedef*)GetCardFeature(type, features); bool error = false;
#define Check(parameter) if (nf.parameter == VK_TRUE) error &= !(src.parameter & VK_TRUE);
#define ECheck(structTypedef) if(error) { NF_ERROR(type); printf("%s X \n", #structTypedef); } else { printf("%s ✓\n", #structTypedef); }

		while (pNext) {
			VkStructureType type = *(VkStructureType*)pNext;

			if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES) {
				Acquire(VkPhysicalDeviceShaderDrawParametersFeatures);
				Check(shaderDrawParameters);
				ECheck(VkPhysicalDeviceShaderDrawParametersFeatures);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES) {
				Acquire(VkPhysicalDeviceVulkan11Features);
				Check(storageBuffer16BitAccess);
				Check(uniformAndStorageBuffer16BitAccess);
				Check(storagePushConstant16);
				Check(storageInputOutput16);
				Check(multiview);
				Check(multiviewGeometryShader);
				Check(multiviewTessellationShader);
				Check(variablePointersStorageBuffer);
				Check(variablePointers);
				Check(protectedMemory);
				Check(samplerYcbcrConversion);
				Check(shaderDrawParameters);
				ECheck(VkPhysicalDeviceVulkan11Features);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES) {
				Acquire(VkPhysicalDeviceVulkan12Features);
				Check(samplerMirrorClampToEdge);
				Check(drawIndirectCount);
				Check(storageBuffer8BitAccess);
				Check(uniformAndStorageBuffer8BitAccess);
				Check(storagePushConstant8);
				Check(shaderBufferInt64Atomics);
				Check(shaderSharedInt64Atomics);
				Check(shaderFloat16);
				Check(shaderInt8);
				Check(descriptorIndexing);
				Check(shaderInputAttachmentArrayDynamicIndexing);
				Check(shaderUniformTexelBufferArrayDynamicIndexing);
				Check(shaderStorageTexelBufferArrayDynamicIndexing);
				Check(shaderUniformBufferArrayNonUniformIndexing);
				Check(shaderSampledImageArrayNonUniformIndexing);
				Check(shaderStorageBufferArrayNonUniformIndexing);
				Check(shaderStorageImageArrayNonUniformIndexing);
				Check(shaderInputAttachmentArrayNonUniformIndexing);
				Check(shaderUniformTexelBufferArrayNonUniformIndexing);
				Check(shaderStorageTexelBufferArrayNonUniformIndexing);
				Check(descriptorBindingUniformBufferUpdateAfterBind);
				Check(descriptorBindingSampledImageUpdateAfterBind);
				Check(descriptorBindingStorageImageUpdateAfterBind);
				Check(descriptorBindingStorageBufferUpdateAfterBind);
				Check(descriptorBindingUniformTexelBufferUpdateAfterBind);
				Check(descriptorBindingStorageTexelBufferUpdateAfterBind);
				Check(descriptorBindingUpdateUnusedWhilePending);
				Check(descriptorBindingPartiallyBound);
				Check(descriptorBindingVariableDescriptorCount);
				Check(runtimeDescriptorArray);
				Check(samplerFilterMinmax);
				Check(scalarBlockLayout);
				Check(imagelessFramebuffer);
				Check(uniformBufferStandardLayout);
				Check(shaderSubgroupExtendedTypes);
				Check(separateDepthStencilLayouts);
				Check(hostQueryReset);
				Check(timelineSemaphore);
				Check(bufferDeviceAddress);
				Check(bufferDeviceAddressCaptureReplay);
				Check(bufferDeviceAddressMultiDevice);
				Check(vulkanMemoryModel);
				Check(vulkanMemoryModelDeviceScope);
				Check(vulkanMemoryModelAvailabilityVisibilityChains);
				Check(shaderOutputViewportIndex);
				Check(shaderOutputLayer);
				Check(subgroupBroadcastDynamicId);
				ECheck(VkPhysicalDeviceVulkan12Features);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES) {
				Acquire(VkPhysicalDevice8BitStorageFeatures);
				Check(storageBuffer8BitAccess);
				Check(uniformAndStorageBuffer8BitAccess);
				Check(storagePushConstant8);
				ECheck(VkPhysicalDevice8BitStorageFeatures);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES) {
				Acquire(VkPhysicalDevice16BitStorageFeatures);
				Check(storageBuffer16BitAccess);
				Check(uniformAndStorageBuffer16BitAccess);
				Check(storagePushConstant16);
				Check(storageInputOutput16);
				ECheck(VkPhysicalDevice16BitStorageFeatures);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES) {
				Acquire(VkPhysicalDeviceDynamicRenderingFeatures);
				Check(dynamicRendering);
				ECheck(VkPhysicalDeviceDynamicRenderingFeatures);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES) {
				Acquire(VkPhysicalDeviceShaderAtomicInt64Features);
				Check(shaderBufferInt64Atomics);
				Check(shaderSharedInt64Atomics);
				ECheck(VkPhysicalDeviceShaderAtomicInt64Features);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES) {
				Acquire(VkPhysicalDeviceShaderFloat16Int8Features);
				Check(shaderFloat16);
				Check(shaderInt8);
				ECheck(VkPhysicalDeviceShaderFloat16Int8Features);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES) {
				Acquire(VkPhysicalDeviceDescriptorIndexingFeatures);
				Check(shaderInputAttachmentArrayDynamicIndexing);
				Check(shaderUniformTexelBufferArrayDynamicIndexing);
				Check(shaderStorageTexelBufferArrayDynamicIndexing);
				Check(shaderUniformBufferArrayNonUniformIndexing);
				Check(shaderSampledImageArrayNonUniformIndexing);
				Check(shaderStorageBufferArrayNonUniformIndexing);
				Check(shaderStorageImageArrayNonUniformIndexing);
				Check(shaderInputAttachmentArrayNonUniformIndexing);
				Check(shaderUniformTexelBufferArrayNonUniformIndexing);
				Check(shaderStorageTexelBufferArrayNonUniformIndexing);
				Check(descriptorBindingUniformBufferUpdateAfterBind);
				Check(descriptorBindingSampledImageUpdateAfterBind);
				Check(descriptorBindingStorageImageUpdateAfterBind);
				Check(descriptorBindingStorageBufferUpdateAfterBind);
				Check(descriptorBindingUniformTexelBufferUpdateAfterBind);
				Check(descriptorBindingStorageTexelBufferUpdateAfterBind);
				Check(descriptorBindingUpdateUnusedWhilePending);
				Check(descriptorBindingPartiallyBound);
				Check(descriptorBindingVariableDescriptorCount);
				Check(runtimeDescriptorArray);
				ECheck(VkPhysicalDeviceDescriptorIndexingFeatures);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES) {
				Acquire(VkPhysicalDeviceScalarBlockLayoutFeatures);
				Check(scalarBlockLayout);
				ECheck(VkPhysicalDeviceScalarBlockLayoutFeatures);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES) {
				Acquire(VkPhysicalDeviceVulkanMemoryModelFeatures);
				Check(vulkanMemoryModel);
				Check(vulkanMemoryModelDeviceScope);
				Check(vulkanMemoryModelAvailabilityVisibilityChains);
				ECheck(VkPhysicalDeviceVulkanMemoryModelFeatures);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES) {
				Acquire(VkPhysicalDeviceImagelessFramebufferFeatures);
				Check(imagelessFramebuffer);
				ECheck(VkPhysicalDeviceImagelessFramebufferFeatures);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES) {
				Acquire(VkPhysicalDeviceUniformBufferStandardLayoutFeatures);
				Check(uniformBufferStandardLayout);
				ECheck(VkPhysicalDeviceUniformBufferStandardLayoutFeatures);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES) {
				Acquire(VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures);
				Check(shaderSubgroupExtendedTypes);
				ECheck(VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES) {
				Acquire(VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures);
				Check(separateDepthStencilLayouts);
				ECheck(VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES) {
				Acquire(VkPhysicalDeviceHostQueryResetFeatures);
				Check(hostQueryReset);
				ECheck(VkPhysicalDeviceHostQueryResetFeatures);
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES) {
				VkPhysicalDeviceTimelineSemaphoreFeatures nf = *(VkPhysicalDeviceTimelineSemaphoreFeatures*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES) {
				VkPhysicalDeviceBufferDeviceAddressFeatures nf = *(VkPhysicalDeviceBufferDeviceAddressFeatures*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES) {
				VkPhysicalDeviceVulkan13Features nf = *(VkPhysicalDeviceVulkan13Features*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES) {
				VkPhysicalDeviceShaderTerminateInvocationFeatures nf = *(VkPhysicalDeviceShaderTerminateInvocationFeatures*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES) {
				VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures nf = *(VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES) {
				VkPhysicalDevicePrivateDataFeatures nf = *(VkPhysicalDevicePrivateDataFeatures*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES) {
				VkPhysicalDevicePipelineCreationCacheControlFeatures nf = *(VkPhysicalDevicePipelineCreationCacheControlFeatures*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES) {
				VkPhysicalDeviceSynchronization2Features nf = *(VkPhysicalDeviceSynchronization2Features*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_WORKGROUP_MEMORY_FEATURES) {
				VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures nf = *(VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES) {
				VkPhysicalDeviceImageRobustnessFeatures nf = *(VkPhysicalDeviceImageRobustnessFeatures*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES) {
				VkPhysicalDeviceSubgroupSizeControlFeatures nf = *(VkPhysicalDeviceSubgroupSizeControlFeatures*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES) {
				VkPhysicalDeviceInlineUniformBlockFeatures nf = *(VkPhysicalDeviceInlineUniformBlockFeatures*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES) {
				VkPhysicalDeviceTextureCompressionASTCHDRFeatures nf = *(VkPhysicalDeviceTextureCompressionASTCHDRFeatures*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES) {
				VkPhysicalDeviceShaderIntegerDotProductFeatures nf = *(VkPhysicalDeviceShaderIntegerDotProductFeatures*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES) {
				VkPhysicalDeviceMaintenance4Features nf = *(VkPhysicalDeviceMaintenance4Features*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR) {
				VkPhysicalDevicePerformanceQueryFeaturesKHR nf = *(VkPhysicalDevicePerformanceQueryFeaturesKHR*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR) {
				VkPhysicalDeviceShaderClockFeaturesKHR nf = *(VkPhysicalDeviceShaderClockFeaturesKHR*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES_EXT) {
				VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR nf = *(VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR) {
				VkPhysicalDeviceFragmentShadingRateFeaturesKHR nf = *(VkPhysicalDeviceFragmentShadingRateFeaturesKHR*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR) {
				VkPhysicalDevicePresentWaitFeaturesKHR nf = *(VkPhysicalDevicePresentWaitFeaturesKHR*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR) {
				VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR nf = *(VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR) {
				VkPhysicalDevicePresentIdFeaturesKHR nf = *(VkPhysicalDevicePresentIdFeaturesKHR*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR) {
				VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR nf = *(VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR) {
				VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR nf = *(VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT) {
				VkPhysicalDeviceTransformFeedbackFeaturesEXT nf = *(VkPhysicalDeviceTransformFeedbackFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV) {
				VkPhysicalDeviceCornerSampledImageFeaturesNV nf = *(VkPhysicalDeviceCornerSampledImageFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT) {
				VkPhysicalDeviceConditionalRenderingFeaturesEXT nf = *(VkPhysicalDeviceConditionalRenderingFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT) {
				VkPhysicalDeviceDepthClipEnableFeaturesEXT nf = *(VkPhysicalDeviceDepthClipEnableFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT) {
				VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT nf = *(VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV) {
				VkPhysicalDeviceShadingRateImageFeaturesNV nf = *(VkPhysicalDeviceShadingRateImageFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT) {
				VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT nf = *(VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV) {
				VkPhysicalDeviceComputeShaderDerivativesFeaturesNV nf = *(VkPhysicalDeviceComputeShaderDerivativesFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV) {
				VkPhysicalDeviceMeshShaderFeaturesNV nf = *(VkPhysicalDeviceMeshShaderFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV) {
				VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV nf = *(VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV) {
				VkPhysicalDeviceShaderImageFootprintFeaturesNV nf = *(VkPhysicalDeviceShaderImageFootprintFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV) {
				VkPhysicalDeviceExclusiveScissorFeaturesNV nf = *(VkPhysicalDeviceExclusiveScissorFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL) {
				VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL nf = *(VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT) {
				VkPhysicalDeviceFragmentDensityMapFeaturesEXT nf = *(VkPhysicalDeviceFragmentDensityMapFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD) {
				VkPhysicalDeviceCoherentMemoryFeaturesAMD nf = *(VkPhysicalDeviceCoherentMemoryFeaturesAMD*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT) {
				VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT nf = *(VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT) {
				VkPhysicalDeviceMemoryPriorityFeaturesEXT nf = *(VkPhysicalDeviceMemoryPriorityFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV) {
				VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV nf = *(VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES) {
				VkPhysicalDeviceBufferDeviceAddressFeaturesEXT nf = *(VkPhysicalDeviceBufferDeviceAddressFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT) {
				VkValidationFeaturesEXT nf = *(VkValidationFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV) {
				VkPhysicalDeviceCooperativeMatrixFeaturesNV nf = *(VkPhysicalDeviceCooperativeMatrixFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV) {
				VkPhysicalDeviceCoverageReductionModeFeaturesNV nf = *(VkPhysicalDeviceCoverageReductionModeFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT) {
				VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT nf = *(VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT) {
				VkPhysicalDeviceYcbcrImageArraysFeaturesEXT nf = *(VkPhysicalDeviceYcbcrImageArraysFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT) {
				VkPhysicalDeviceLineRasterizationFeaturesEXT nf = *(VkPhysicalDeviceLineRasterizationFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT) {
				VkPhysicalDeviceShaderAtomicFloatFeaturesEXT nf = *(VkPhysicalDeviceShaderAtomicFloatFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT) {
				VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT nf = *(VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV) {
				VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV nf = *(VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV) {
				VkPhysicalDeviceInheritedViewportScissorFeaturesNV nf = *(VkPhysicalDeviceInheritedViewportScissorFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT) {
				VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT nf = *(VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_MEMORY_REPORT_FEATURES_EXT) {
				VkPhysicalDeviceDeviceMemoryReportFeaturesEXT nf = *(VkPhysicalDeviceDeviceMemoryReportFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT) {
				VkPhysicalDeviceRobustness2FeaturesEXT nf = *(VkPhysicalDeviceRobustness2FeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT) {
				VkPhysicalDeviceCustomBorderColorFeaturesEXT nf = *(VkPhysicalDeviceCustomBorderColorFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_FEATURES_NV) {
				VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV nf = *(VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV) {
				VkPhysicalDeviceRayTracingMotionBlurFeaturesNV nf = *(VkPhysicalDeviceRayTracingMotionBlurFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_2_PLANE_444_FORMATS_FEATURES_EXT) {
				VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT nf = *(VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT) {
				VkPhysicalDeviceFragmentDensityMap2FeaturesEXT nf = *(VkPhysicalDeviceFragmentDensityMap2FeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT) {
				VkPhysicalDevice4444FormatsFeaturesEXT nf = *(VkPhysicalDevice4444FormatsFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_ARM) {
				VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesARM nf = *(VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesARM*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RGBA10X6_FORMATS_FEATURES_EXT) {
				VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT nf = *(VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_VALVE) {
				VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE nf = *(VkPhysicalDeviceMutableDescriptorTypeFeaturesVALVE*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT) {
				VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT nf = *(VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT) {
				VkPhysicalDeviceDepthClipControlFeaturesEXT nf = *(VkPhysicalDeviceDepthClipControlFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT) {
				VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT nf = *(VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_FEATURES_HUAWEI) {
				VkPhysicalDeviceSubpassShadingFeaturesHUAWEI nf = *(VkPhysicalDeviceSubpassShadingFeaturesHUAWEI*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_RDMA_FEATURES_NV) {
				VkPhysicalDeviceExternalMemoryRDMAFeaturesNV nf = *(VkPhysicalDeviceExternalMemoryRDMAFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT) {
				VkPhysicalDeviceExtendedDynamicState2FeaturesEXT nf = *(VkPhysicalDeviceExtendedDynamicState2FeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT) {
				VkPhysicalDeviceColorWriteEnableFeaturesEXT nf = *(VkPhysicalDeviceColorWriteEnableFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT) {
				VkPhysicalDeviceImageViewMinLodFeaturesEXT nf = *(VkPhysicalDeviceImageViewMinLodFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES) {
				VkPhysicalDeviceMultiDrawFeaturesEXT nf = *(VkPhysicalDeviceMultiDrawFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BORDER_COLOR_SWIZZLE_FEATURES_EXT) {
				VkPhysicalDeviceBorderColorSwizzleFeaturesEXT nf = *(VkPhysicalDeviceBorderColorSwizzleFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT) {
				VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT nf = *(VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINEAR_COLOR_ATTACHMENT_FEATURES_NV) {
				VkPhysicalDeviceLinearColorAttachmentFeaturesNV nf = *(VkPhysicalDeviceLinearColorAttachmentFeaturesNV*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR) {
				VkPhysicalDeviceAccelerationStructureFeaturesKHR nf = *(VkPhysicalDeviceAccelerationStructureFeaturesKHR*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR) {
				VkPhysicalDeviceRayTracingPipelineFeaturesKHR nf = *(VkPhysicalDeviceRayTracingPipelineFeaturesKHR*)pNext;
			}
			else if (type == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR) {
				VkPhysicalDeviceRayQueryFeaturesKHR nf = *(VkPhysicalDeviceRayQueryFeaturesKHR*)pNext;
			}
			else {
				char particle_buffer[512]{ 0 };
				sprintf(particle_buffer, "Unknown Feature requested VkStructureType: %d --- 0x%x, 0X%X\n", type, type, type);
				LOG(WARNING, particle_buffer, true);
			}

			// use any structure to get the next pNext
			pNext = ((VkPhysicalDeviceVulkan11Features*)pNext)->pNext;
		}
		return MeetSpecification;
	}
}