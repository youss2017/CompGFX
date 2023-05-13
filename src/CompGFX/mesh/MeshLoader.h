#pragma once
#include <vector>
#include <memory>
#include <string>

namespace egx {

	enum class VertexDataOrder {
		Position, Normal, UV, Tangent, Bitangent
	};

	enum class IndicesType {
		Int8, Int16, Int32
	};

	class MeshContainer {
	public:
		MeshContainer() = default;
		MeshContainer& Load(const std::string& file, IndicesType type, const std::initializer_list<VertexDataOrder>& vertexDataOrder);
		std::vector<float>& Vertices() const;
		std::vector<uint8_t>& Indices32() const;
		std::vector<uint8_t>& Indices16() const;
		std::vector<uint8_t>& Indices8() const;
		uint32_t MeshCount() const;

	protected:
		struct DataWrapper {
			std::vector<float> m_Vertices;
			std::vector<uint32_t> m_Indice32;
			std::vector<uint16_t> m_Indice16;
			std::vector<uint8_t> m_Indice8;
		};
		std::vector<std::unique_ptr<DataWrapper>> m_MeshData;
	};

}
