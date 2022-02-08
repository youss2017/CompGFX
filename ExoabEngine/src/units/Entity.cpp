#include "Entity.hpp"

void EntityBindingData::AddElement(const std::string &name, EntityDataType type, int elementOffsetInStruct, void *pData)
{
	BindingData data;
	data.name = name;
	data.type = type;
	data.elementOffsetInStruct = elementOffsetInStruct;
	switch (type)
	{
	case ENTITY_DATA_TYPE_MAT4:
		data.elementSize = sizeof(glm::mat4);
		break;
	case ENTITY_DATA_TYPE_MAT3:
		data.elementSize = sizeof(glm::mat3);
		break;
	case ENTITY_DATA_TYPE_MAT2:
		data.elementSize = sizeof(glm::mat2);
		break;
	case ENTITY_DATA_TYPE_FLOAT:
	case ENTITY_DATA_TYPE_INT:
	case ENTITY_DATA_TYPE_UINT:
		data.elementSize = 4;
		break;
	default:
		Utils::Break();
	}
	if (pData)
		memcpy(&data.dataunion, pData, data.elementSize);
	bindingData.push_back(data);
}

DefaultEntity *Entity_CreateDefaultEntity(GraphicsContext context, const char *mesh_path)
{
	DefaultEntity *ent = new DefaultEntity();
	ent->m_model = std::make_unique<OmegaModel<OmegaBasicVertex>>(context, mesh_path);
	return ent;
}