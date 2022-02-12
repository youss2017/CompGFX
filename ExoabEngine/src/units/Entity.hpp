#pragma once
#include "../mesh/Model.hpp"
#include <string>
#include <vector>
#include <memory>
#include <string_view>
#include <glm/glm.hpp>
#include <cassert>

enum EntityDataType
{
	ENTITY_DATA_TYPE_MAT4,
	ENTITY_DATA_TYPE_MAT3,
	ENTITY_DATA_TYPE_MAT2,
	ENTITY_DATA_TYPE_FLOAT,
	ENTITY_DATA_TYPE_INT,
	ENTITY_DATA_TYPE_UINT,
};

// TODO: Would char[] instead of std::string be faster?
// TODO: Support arrays
class EntityBindingData
{
public:
	EntityBindingData(int setID, int bindingID)
	: setID(setID), bindingID(bindingID)
	{
		bindingData.reserve(10);
	}

	int setID;
	int bindingID;
	struct BindingData
	{
		std::string name;
		EntityDataType type;
		int elementOffsetInStruct;
		int elementSize;
		union {
			glm::mat4 matrix4;
			glm::mat3 matrix3;
			glm::mat2 matrix2;
			float float32;
			int int32;
			unsigned int uint32;
		} dataunion;
	};

	void AddElement(const std::string& name, EntityDataType type, int elementOffsetInStruct, void* pData);

	inline BindingData& operator[](const int& index)
	{
		assert(index >= 0 && index < bindingData.size());
		return bindingData[index];
	}

	inline const size_t size() { return bindingData.size(); }

	inline bool operator==(const EntityBindingData& otherBinding)
	{
		return otherBinding.setID == setID && otherBinding.bindingID == bindingID;
	}

	std::vector<BindingData> bindingData;

};


class DefaultEntity
{
public:
	std::shared_ptr<OmegaModel<OmegaBasicVertex>> m_model;
	std::vector<VkDeviceSize> VertexInstanceBufferOffset;
	std::vector<IBuffer2> VertexInstanceBuffers;
	std::vector<VkBuffer> VkVertexInstanceBuffers;
	uint32_t DrawInstanceCount = 1;
	// This flag is used to update bound buffer for OpenGL
	// Since in opengl we need to bind a buffer to setup instancing.
	bool m_InstanceBufferUpdated = true;

	static DefaultEntity CreateChildEntity(DefaultEntity* host)
	{
		DefaultEntity child;
		child.m_model = host->m_model;
		child.shader_data = host->shader_data;
		return child;
	}

	void SetInstanceCount(uint32_t InstanceCount)
	{
		DrawInstanceCount = std::max(InstanceCount, 1u);
	}

	void AddBindingData(const EntityBindingData& bindingData)
	{
		std::vector<EntityBindingData>::iterator e = std::find(shader_data.begin(), shader_data.end(), bindingData);
		assert(e == shader_data.end());
		shader_data.push_back(bindingData);
		shader_data.shrink_to_fit();
	}

	// Add In Order from lowest to highest bindingID
	void AddVertexInstanceBuffer(IBuffer2 buffer, VkDeviceSize Offset = 0)
	{
		VertexInstanceBuffers.push_back(buffer);
		VkVertexInstanceBuffers.push_back(buffer->m_vk_buffer->m_buffer);
		VertexInstanceBufferOffset.push_back(Offset);
	}

	inline EntityBindingData& operator[](const int& index)
	{
		return shader_data[index];
	}

	inline EntityBindingData& get(const int& index)
	{
		assert(index >= 0 && index < shader_data.size());
		return shader_data[index];
	}

	const inline size_t size() { return shader_data.size(); }
	
private:
	std::vector<EntityBindingData> shader_data;
};

DefaultEntity* Entity_CreateDefaultEntity(GraphicsContext context, const char* mesh_path);

