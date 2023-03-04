#pragma once
#include <core/egx.hpp>
#include <glm/glm.hpp>
#include <vector>
// Draws lines, cubes, and spheres (fill and line-mode)

namespace egx::util {

	namespace internal {
		enum class DrawType : int {
			LINE,
			QUAD,
			CUBE,
			CIRCLE,
			SPHERE,
			UNDEFINED
		};

		struct DrawInstruction
		{
			DrawType Type{DrawType::UNDEFINED};
			glm::vec3 a{};
			glm::vec3 b{};
			glm::vec3 color{1.0,1.0,1.0};
			glm::vec2 size{};
			float radius{};
			float thickness{};
			int ringCount{};
		};
	}

	class DebugRenderer
	{
	public:
		EGX_API DebugRenderer(const egx::ref<VulkanCoreInterface>& coreInterface, const egx::ref<egx::Framebuffer>& framebuffer, uint32_t passId, std::optional<uint32_t> entityId = {});

		void EGX_API ClearCommands();
		void EGX_API DrawLine(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color, float thickness = 1.0f);
		void EGX_API DrawQuad(const glm::vec3& center, const glm::vec2& size, const glm::vec3& color);
		void EGX_API DrawCircle(const glm::vec3& p, float radius, const glm::vec3& color, float thickness = 1.0f);
		void EGX_API DrawSphereOutline(const glm::vec3& p, float radius, const glm::vec3& color, int ringCount = 4);
		void EGX_API WriteCommands(VkCommandBuffer cmd);

		inline void UpdateProjView(const glm::mat4& proj, const glm::mat4& view) { m_ffProj = proj, m_ffView = view; }

		void EGX_API SetFramebuffer(const egx::ref<egx::Framebuffer>& framebuffer, uint32_t passId);

		glm::vec3 cameraPos{};

	private:
		egx::ref<egx::VulkanCoreInterface> m_CoreInterface;
		std::vector<internal::DrawInstruction> m_vsInstructions;
		glm::mat4 m_ffProj{1.0};
		glm::mat4 m_ffView{1.0};
		egx::ref<egx::Buffer> m_bfVertices;
		egx::ref<egx::Buffer> m_bfIndices;
		egx::ref<egx::Buffer> m_bfInstanceBuffer;
		egx::ref<egx::Pipeline> m_plDebug;
		std::optional<uint32_t> m_iEntityId;
	};

}