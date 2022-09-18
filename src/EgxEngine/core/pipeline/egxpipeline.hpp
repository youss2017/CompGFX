#pragma once
#include "../egxcommon.hpp"
#include "../shaders/egxshader.hpp"
#include "../shaders/egxshaderdata.hpp"
#include "../memory/egxref.hpp"
#include "egxframebuffer.hpp"
#include <set>

namespace egx {

	class egxvertexdescription {

	public:
		egxvertexdescription(bool IsAligned = true) : IsAligned(IsAligned) {};
		egxvertexdescription(egxvertexdescription& copy) = default;
		egxvertexdescription(egxvertexdescription&& move) = default;
		egxvertexdescription& operator=(egxvertexdescription&& move) = default;

		inline void AddAttribute(
			uint32_t BindingId, uint32_t Location,
			uint32_t Offset, uint32_t Size,
			VkFormat Format) {
			Attributes.push_back(
				{BindingId, Location, Offset, Size, Format}
			);
		}

		inline uint32_t GetStride(uint32_t BindingId) const {
			uint32_t stride = 0;
			for (auto& e : Attributes) {
				if (e.BindingId != BindingId) continue;
				if (IsAligned) {
					auto size = e.Size;
					size -= size % 4;
					size += 4;
					stride += size;
				}
				else {
					stride += e.Size;
				}
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
		static ref<Pipeline> EGX_API FactoryCreate(ref<VulkanCoreInterface>& CoreInterface);
		EGX_API ~Pipeline();
		EGX_API Pipeline(Pipeline& copy) = delete;
		EGX_API Pipeline(Pipeline&& move) noexcept;
		EGX_API Pipeline& operator=(Pipeline&& move) noexcept;

		void EGX_API create(
			const ref<PipelineLayout>& layout,
			const egxshader& vertex,
			const egxshader& fragment,
			const ref<egxframebuffer>& framebuffer,
			const egxvertexdescription& vertexDescription);

		void EGX_API create(const ref<PipelineLayout>& layout, const egxshader& compute);

		inline void bind(VkCommandBuffer cmd) const {
			vkCmdBindPipeline(cmd, _graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE, Pipe);
		}

		inline VkPipeline operator()() const { return Pipe; }
		inline VkPipeline operator*() const { return Pipe; }

	protected:
		Pipeline(ref<VulkanCoreInterface>& coreinterface) :
			_coreinterface(coreinterface) {}

	public:
		/// Pipeline properties you do not have to set viewport width/height
		// the default is to use the framebuffer
		cullmode CullMode = cullmode_back;
		frontface FrontFace = frontface_ccw;
		depthcompare DepthCompare = depthcompare_less;
		polygonmode FillMode = polygonmode_fill;
		polygontopology Topology = polgyontopology_trianglelist;
		float NearField = 0.0f;
		float FarField = 1.0f;
		float LineWidth = 1.0f;
		uint32_t ViewportWidth = 0;
		uint32_t ViewportHeight = 0;
		VkPipeline Pipe = nullptr;
		bool DepthEnabled = true;
		bool DepthWriteEnable = true;

	protected:
		ref<VulkanCoreInterface> _coreinterface;
		bool _graphics = false;

	};

}