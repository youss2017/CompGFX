#pragma once
#include "../egxcommon.hpp"
#include "../shaders/egxshader.hpp"
#include "../shaders/egxshaderset.hpp"
#include "../memory/egxref.hpp"
#include "egxframebuffer.hpp"
#include <set>

namespace egx {

	class InputAssemblyDescription {

	public:
		InputAssemblyDescription(bool IsAligned = true) : IsAligned(IsAligned) {};
		InputAssemblyDescription(InputAssemblyDescription& copy) = default;
		InputAssemblyDescription(InputAssemblyDescription&& move) = default;
		InputAssemblyDescription& operator=(InputAssemblyDescription&& move) = default;

		inline void AddAttribute(
			uint32_t BindingId, uint32_t Location,
			uint32_t Size, VkFormat Format) {
			uint32_t offset = 0;
			for (auto& a : Attributes)
				if ((a.BindingId == BindingId))
					offset += a.Size;
			Attributes.push_back(
				{ BindingId, Location, offset, Size, Format }
			);
		}

		inline uint32_t GetStride(uint32_t BindingId) const {
			uint32_t stride = 0;
			for (auto& e : Attributes) {
				if (e.BindingId != BindingId) continue;
				stride += e.Size;
			}
			return stride;
		}

		inline std::set<uint32_t> GetBindings() const {
			std::set<uint32_t> bindings;
			for (auto& a : Attributes) {
				bindings.insert(a.BindingId);
			}
			return bindings;
		}

	public:
		struct attribute {
			uint32_t BindingId;
			uint32_t Location;
			uint32_t Offset;
			uint32_t Size;
			VkFormat Format;
		};
		std::vector<attribute> Attributes;
		bool IsAligned;
	};

	enum cullmode : uint32_t {
		cullmode_none,
		cullmode_back,
		cullmode_front,
		cullmode_front_and_back
	};

	enum frontface : uint32_t {
		frontface_cw,
		frontface_ccw
	};

	enum depthcompare : uint32_t {
		depthcompare_never,
		depthcompare_less,
		depthcompare_equal,
		depthcompare_less_equal,
		depthcompare_greater,
		depthcompare_not_equal,
		depthcompare_greater_equal,
		depthcompare_always,
	};

	enum polygonmode : uint32_t {
		polygonmode_point,
		polygonmode_line,
		polygonmode_fill
	};

	enum polygontopology : uint32_t {
		polgyontopology_trianglelist,
		polgyontopology_linelist,
		polgyontopology_linestrip,
		polgyontopology_pointlist,
	};

	class Pipeline {
	public:
		static ref<Pipeline> EGX_API FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface);
		EGX_API ~Pipeline();
		EGX_API Pipeline(Pipeline& copy) = delete;
		EGX_API Pipeline(Pipeline&& move) noexcept;
		EGX_API Pipeline& operator=(Pipeline&& move) noexcept;

		/// <summary>
		/// PassId is determined from Framebuffer::CreatePass()
		/// </summary>
		/// <param name="layout"></param>
		/// <param name="vertex"></param>
		/// <param name="fragment"></param>
		/// <param name="framebuffer"></param>
		/// <param name="vertexDescription"></param>
		/// <returns></returns>
		void EGX_API invalidate(
			const ref<PipelineLayout>& layout,
			const Shader& vertex,
			const Shader& fragment,
			const ref<Framebuffer>& framebuffer,
			const uint32_t PassId,
			const InputAssemblyDescription& vertexDescription);

		void EGX_API invalidate(const ref<PipelineLayout>& layout, const Shader& compute);

		inline void Bind(VkCommandBuffer cmd) const {
			vkCmdBindPipeline(cmd, _graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE, Pipe);
		}

		inline void Bind(VkCommandBuffer cmd, const ref<PipelineLayout> Layout) const
		{
			vkCmdBindPipeline(cmd, _graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE, Pipe);
			Layout->Bind(cmd, _graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE);
		}

		inline VkPipeline operator()() const { return Pipe; }
		inline VkPipeline operator*() const { return Pipe; }

	protected:
		Pipeline(const ref<VulkanCoreInterface>& coreinterface) :
			_coreinterface(coreinterface) {}

	public:
		/// Pipeline properties you do not have to set viewport width/height
		// the default is to use the framebuffer
		cullmode CullMode = cullmode_none;
		frontface FrontFace = frontface_ccw;
		depthcompare DepthCompare = depthcompare_always;
		polygonmode FillMode = polygonmode_fill;
		polygontopology Topology = polgyontopology_trianglelist;
		float NearField = 0.0f;
		float FarField = 1.0f;
		float LineWidth = 1.0f;
		uint32_t ViewportWidth = 0;
		uint32_t ViewportHeight = 0;
		VkPipeline Pipe = nullptr;
		bool DepthEnabled = false;
		bool DepthWriteEnable = true;

	protected:
		ref<VulkanCoreInterface> _coreinterface;
		bool _graphics = false;

	};

}