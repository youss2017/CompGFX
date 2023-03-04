#include "DebugRenderer.hpp"
#include <glm/gtc/matrix_transform.hpp>

using namespace egx;
using namespace egx::util;
using namespace egx::util::internal;
using namespace glm;

struct Pushblock {
	glm::mat4 projView;
};

struct DebugVertex {
	glm::vec3 InPos;
	glm::vec3 InColor;
	glm::vec3 InThicknessFadeAndCircleFlag;
	glm::vec2 InUv;
};

static const char* debugVS = R"(
#version 450 core
layout (location = 0) in vec3 InPos;
layout (location = 1) in vec3 InColor;
layout (location = 2) in vec3 InThicknessFadeAndCircleFlag;
layout (location = 3) in vec2 InUv;

layout (push_constant) uniform pushblock {
	mat4 projView;
};

layout (location = 0) out vec2 UV;
layout (location = 1) out vec3 Color;
layout (location = 2) out flat int CircleFlag;
layout (location = 3) out float Thickness;
layout (location = 4) out float Fade;

void main()
{
	UV = InUv;
	Color = InColor;
	Thickness = InThicknessFadeAndCircleFlag[0];
	Fade = InThicknessFadeAndCircleFlag[1];
	CircleFlag = int(InThicknessFadeAndCircleFlag[2]);
	gl_Position = projView * vec4(InPos, 1.0);
}
)";

static const char* debugFS = R"(
#version 450 core
layout (location = 0) out vec4 FragColor;
#ifdef DEBUG_ENTITY_ID
layout (location = 1) out float EntityID;
#endif

layout (location = 0) in vec2 UV;
layout (location = 1) in vec3 Color;
layout (location = 2) in flat int CircleFlag;
layout (location = 3) in float thickness;
layout (location = 4) in float fade;

void main()
{
	if(CircleFlag == 1) {
		vec2 uv = UV * 2.0 - 1.0;
		float distance = 1.0 - length(uv);
		vec3 col = vec3(smoothstep(0.0, fade, distance));
		col *= vec3(smoothstep(thickness + fade, thickness, distance));
		if(col[0] > 0.0) {
			col *= Color;
			FragColor = vec4(col, 1.0);
		} else {
			discard;
			return;
		}
	} else {
		FragColor = vec4(Color, 1.0);
	}
#ifdef DEBUG_ENTITY_ID
	EntityID = DEBUG_ENTITY_ID;
#endif
}
)";

DebugRenderer::DebugRenderer(const egx::ref<VulkanCoreInterface>& coreInterface, const egx::ref<egx::Framebuffer>& framebuffer, uint32_t passId, std::optional<uint32_t> entityId)
	: m_iEntityId(entityId), m_CoreInterface(coreInterface)
{
	m_bfVertices = Buffer::FactoryCreate(coreInterface, sizeof(DebugVertex) * 10, egx::memorylayout::dynamic, BufferType_Vertex, true, false);
	m_bfIndices = Buffer::FactoryCreate(coreInterface, 1000, egx::memorylayout::local, BufferType_Index, false, false);
	SetFramebuffer(framebuffer, passId);
}

void DebugRenderer::SetFramebuffer(const egx::ref<egx::Framebuffer>& framebuffer, uint32_t passId)
{
	Shader2Defines defines;
	if (m_iEntityId) {
		defines.Add("DEBUG_ENTITY_ID", std::to_string(*m_iEntityId));
	}

	auto vs = Shader2::FactoryCreateEx(m_CoreInterface, debugVS, VK_SHADER_STAGE_VERTEX_BIT, egx::BindingAttributes::Default, "", "Debug VS.vert");
	auto fs = Shader2::FactoryCreateEx(m_CoreInterface, debugFS, VK_SHADER_STAGE_FRAGMENT_BIT, egx::BindingAttributes::Default, "", "Debug FS.vert", defines);
	m_plDebug = egx::Pipeline::FactoryCreate(m_CoreInterface);
	m_plDebug->DepthEnabled = true;
	m_plDebug->FillMode = VK_POLYGON_MODE_FILL;
	m_plDebug->DepthCompare = VK_COMPARE_OP_LESS;
	m_plDebug->Create(vs, fs, framebuffer, passId);
}

void DebugRenderer::ClearCommands()
{
	m_vsInstructions.clear();
}

void DebugRenderer::WriteCommands(VkCommandBuffer cmd)
{
	if (m_vsInstructions.size() == 0) return;
	std::vector<DebugVertex> vertices;
	std::vector<uint32_t> indices;

	vertices.reserve(m_vsInstructions.size() * 4);
	indices.reserve(m_vsInstructions.size() * 6);
	uint32_t nextIndex = 0;

	for (size_t i = 0; i < m_vsInstructions.size(); i++) {
		const auto& instruction = m_vsInstructions[i];
		uint32_t baseIndex = nextIndex;
		if (instruction.Type == internal::DrawType::QUAD ||
			instruction.Type == internal::DrawType::CIRCLE ||
			instruction.Type == internal::DrawType::SPHERE) {
			auto& center = instruction.a;
			const auto& size =
				(instruction.Type == internal::DrawType::QUAD) ? instruction.size : glm::vec2(instruction.radius);

			DebugVertex v0{}, v1{}, v2{}, v3{};
			v0.InColor = v1.InColor = v2.InColor = v3.InColor = instruction.color;
			glm::vec3 c0 = glm::vec3(-size.x / 2.0, -size.y / 2.0, 0.0);
			glm::vec3 c1 = glm::vec3(-size.x / 2.0, +size.y / 2.0, 0.0);
			glm::vec3 c2 = glm::vec3(+size.x / 2.0, +size.y / 2.0, 0.0);
			glm::vec3 c3 = glm::vec3(+size.x / 2.0, -size.y / 2.0, 0.0);
			v0.InPos = center + c0;
			v1.InPos = center + c1;
			v2.InPos = center + c2;
			v3.InPos = center + c3;
			v0.InUv = glm::vec2(0.0, 0.0);
			v1.InUv = glm::vec2(0.0, 1.0);
			v2.InUv = glm::vec2(1.0, 1.0);
			v3.InUv = glm::vec2(1.0, 0.0);

			if (instruction.Type == internal::DrawType::CIRCLE
				|| instruction.Type == internal::DrawType::SPHERE) {
				v0.InThicknessFadeAndCircleFlag =
					v1.InThicknessFadeAndCircleFlag =
					v2.InThicknessFadeAndCircleFlag =
					v3.InThicknessFadeAndCircleFlag = glm::vec3(instruction.thickness, 0.005, 1.0);
			}
			else {
				v0.InThicknessFadeAndCircleFlag = v1.InThicknessFadeAndCircleFlag = v2.InThicknessFadeAndCircleFlag = v3.InThicknessFadeAndCircleFlag = {};
			}
			uint32_t i0, i1, i2, i3, i4, i5;

			if (instruction.Type == internal::DrawType::SPHERE) {
				float angleOffset = 360.0f / float(instruction.ringCount);
				for (float angle = angleOffset; angle <= 360.0f; angle += angleOffset) {
					glm::mat4 transform = glm::translate(glm::mat4(1.0f), center) * glm::rotate(glm::mat4(1.0f), glm::radians<float>(angle), glm::vec3(0.0, 1.0, 0.0));
					v0.InPos = transform * glm::vec4(c0, 1.0);
					v1.InPos = transform * glm::vec4(c1, 1.0);
					v2.InPos = transform * glm::vec4(c2, 1.0);
					v3.InPos = transform * glm::vec4(c3, 1.0);

					baseIndex = nextIndex;

					i0 = baseIndex + 0;
					i1 = baseIndex + 1;
					i2 = baseIndex + 2;
					i3 = baseIndex + 0;
					i4 = baseIndex + 2;
					i5 = baseIndex + 3;

					nextIndex = i5 + 1;

					vertices.push_back(v0), vertices.push_back(v1), vertices.push_back(v2), vertices.push_back(v3);
					indices.push_back(i0), indices.push_back(i1), indices.push_back(i2),
						indices.push_back(i3), indices.push_back(i4), indices.push_back(i5);
				}
			}
			else {
				i0 = baseIndex + 0;
				i1 = baseIndex + 1;
				i2 = baseIndex + 2;
				i3 = baseIndex + 0;
				i4 = baseIndex + 2;
				i5 = baseIndex + 3;

				nextIndex = i5 + 1;

				vertices.push_back(v0), vertices.push_back(v1), vertices.push_back(v2), vertices.push_back(v3);
				indices.push_back(i0), indices.push_back(i1), indices.push_back(i2),
					indices.push_back(i3), indices.push_back(i4), indices.push_back(i5);
			}

		}
	}

	m_bfVertices->Resize(vertices.size() * sizeof(DebugVertex), false);
	m_bfIndices->Resize(indices.size() * sizeof(uint32_t), false);

	m_bfVertices->Write(vertices.data());
	m_bfIndices->Write(indices.data());

	VkDeviceSize offsets[1]{};
	
	auto mvp = m_ffProj * m_ffView;
	m_plDebug->Bind(cmd);
	vkCmdBindVertexBuffers(cmd, 0, 1, &m_bfVertices->GetBuffer(), offsets);
	vkCmdBindIndexBuffer(cmd, m_bfIndices->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
	m_plDebug->PushConstants(cmd, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);
	vkCmdDrawIndexed(cmd, uint32_t(indices.size()), 1, 0, 0, 0);

}

void DebugRenderer::DrawLine(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color, float thickness)
{
	DrawInstruction ins;
	ins.Type = DrawType::LINE;
	ins.a = a, ins.b = b, ins.color = color, ins.thickness = thickness;
	m_vsInstructions.push_back(ins);
}

void DebugRenderer::DrawQuad(const glm::vec3& center, const glm::vec2& size, const glm::vec3& color)
{
	DrawInstruction ins;
	ins.Type = DrawType::QUAD;
	ins.a = center, ins.size = size, ins.color = color;
	m_vsInstructions.push_back(ins);
}

void DebugRenderer::DrawCircle(const glm::vec3& p, float radius, const glm::vec3& color, float thickness)
{
	DrawInstruction ins;
	ins.Type = DrawType::CIRCLE;
	ins.a = p, ins.b = p, ins.radius = radius, ins.color = color, ins.thickness = thickness;
	m_vsInstructions.push_back(ins);
}

void DebugRenderer::DrawSphereOutline(const glm::vec3& p, float radius, const glm::vec3& color, int ringCount)
{
	float thickness = 0.045f;
	DrawInstruction ins;
	ins.Type = DrawType::SPHERE;
	ins.a = p, ins.b = p, ins.radius = radius, ins.color = color, ins.thickness = thickness, ins.ringCount = glm::max(ringCount, 1);
	m_vsInstructions.push_back(ins);
}
