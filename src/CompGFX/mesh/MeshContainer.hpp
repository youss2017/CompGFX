#pragma once
#include <vector>
#include <memory>
#include <string>
#include <core/egx.hpp>
#include <memory/egxbuffer.hpp>
#include <glm/glm.hpp>

namespace egx {

	enum class VertexDataOrder : uint32_t {
		Position, Normal, UV, Tangent, Bitangent
	};

	enum class IndicesType {
		UInt16, UInt32
	};

	class MeshContainer {
	public:
		MeshContainer() = default;
		virtual MeshContainer& Load(const std::string& file, IndicesType type, const std::vector<VertexDataOrder>& vertexDataOrder);
		std::vector<float>& Vertices(uint32_t meshId = 0) const;
		std::vector<uint32_t>& Indices32(uint32_t meshId = 0) const;
		std::vector<uint16_t>& Indices16(uint32_t meshId = 0) const;
		IndicesType GetIndicesType() const;
		uint32_t MeshCount() const;
		uint32_t GetVerticesCount(uint32_t meshId = 0) const;
		uint32_t GetIndicesCount(uint32_t meshId = 0) const;

	protected:
		struct Mesh {
			std::vector<float> m_Vertices;
			std::vector<uint32_t> m_Indice32;
			std::vector<uint16_t> m_Indice16;
			uint32_t m_VerticesCount;
			uint32_t m_IndicesCount;
		};
		IndicesType m_IndicesType = IndicesType::UInt32;
		std::vector<std::unique_ptr<Mesh>> m_MeshData;
	};

	class BufferedMeshContainer : public MeshContainer {
	public:
		BufferedMeshContainer() = default;
		BufferedMeshContainer(const DeviceCtx& ctx) {
			m_Data = std::make_shared<DataWrapper>();
			m_Data->m_Ctx = ctx;
		}

		virtual MeshContainer& Load(const std::string& file, IndicesType type, const std::vector<VertexDataOrder>& vertexDataOrder) override;
		virtual Buffer GetVertexBuffer(uint32_t id = 0) const;
		virtual Buffer GetIndexBuffer(uint32_t id = 0) const;

		/// <summary>
		/// Frees memory used by vertices/indices on the CPU RAM, this is useful
		/// because once the model is loaded, the data is already stored on the GPU.
		/// </summary>
		void ReleaseCPUData();

	protected:
		struct DataWrapper {
			DeviceCtx m_Ctx;
			std::vector<Buffer> m_VertexBuffers;
			std::vector<Buffer> m_IndexBuffers;
		};
		std::shared_ptr<DataWrapper> m_Data;
	};

	class ModelContainer : public BufferedMeshContainer {
	public:
		ModelContainer() = default;
		ModelContainer(const DeviceCtx& ctx) : BufferedMeshContainer(ctx) {}

		void UpdateTransform();

	public:
		glm::mat4 Transform{ 1.0f };
		glm::vec3 Rotation{ 1.0f };
		glm::vec3 Position{ 0.0f };
		glm::vec3 Scaling{ 1.0f };
		// (TODO) Include Material
	};

}
