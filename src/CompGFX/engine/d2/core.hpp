#pragma once
#include <string>
#include <vector>
#include <list>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <memory>
#include "../../pipeline/RenderTarget.hpp"
#include "../../pipeline/pipeline.hpp"
#include "../../pipeline/ShaderBinding.hpp"

namespace egx::d2 {

	enum class ImageMemoryType {
		GRAYSCALE,
		RGBA,
		RGB,
		DEFAULT = RGBA,
	};

	enum class BodyType {
		Sprite,
		Rect,
		Circle
	};

	enum class TrajType {
		Null,
		Linear
	};

	struct Trajectory {
		glm::vec2 destination;
		TrajType trajType;
	};

	class Color {
	public:
		// Constructors
		Color() {
			rgba[0] = 0;
			rgba[1] = 0;
			rgba[2] = 0;
			rgba[3] = 255;
		}


		Color(int32_t red, int32_t green, int32_t blue, int32_t alpha = 255) {
			rgba[0] = red;
			rgba[1] = green;
			rgba[2] = blue;
			rgba[3] = alpha;
		}

		Color(float red, float green, float blue, float alpha = 1.0f) {
			rgba[0] = static_cast<uint8_t>(red * 255);
			rgba[1] = static_cast<uint8_t>(green * 255);
			rgba[2] = static_cast<uint8_t>(blue * 255);
			rgba[3] = static_cast<uint8_t>(alpha * 255);
		}
		static Color Random();
		static Color FromRGB(uint8_t red, uint8_t green, uint8_t blue);
		static Color FromRGB(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);
		static Color FromRGB(float red, float green, float blue);
		static Color FromRGB(float red, float green, float blue, float alpha);
		uint32_t ToInt32();
		glm::vec3 ToVec3();
		uint8_t rgba[4];
		static Color Black;
		static Color Pink;
		static Color Orange;
		static Color Blue;
		static Color Red;
		static Color Yellow;
		static Color Green;
		static Color White;
		static Color Brown;
		static Color Silver;
		static Color Teal;
		static Color Violet;
		static Color Purple;
		static Color Magenta;
		static Color Cyan;
		static Color Grey;
		static Color Olive;
		static Color NavyBlue;
	};

	class Body {
	public:
		Body() {}
		virtual BodyType GetType() { return BodyType::Rect; }
		virtual void NextAnimation(float deltaTime);

		virtual Body& SetOpacity(float opacity) { this->opacity = opacity; return *this; }
		virtual Body& SetColor(Color color) { this->color = color; return *this; }
		
	public:
		glm::vec2 size;
		glm::vec2 position;
		glm::vec2 velocity;
		glm::vec2 acceleration;
		std::vector<Trajectory> trajectories;
		
		glm::vec2 rotation;
		union {
			Color color;
			float uv[3]; // last element is dummy
		};
		uint32_t zindex;
		float opacity;
		float lifetime;
		bool fill;
	private:
		Trajectory m_current_trajectory;
	};

	struct DrawCall {
		glm::vec3 position;
		int flag;
		glm::vec3 color_or_uv[3];
		float opacity;
	};

	class RectBody : public Body {
	public:
		virtual BodyType GetType() override { return BodyType::Rect; }
	};

	class CircleBody : public Body {
	public:
		virtual BodyType GetType() override { return BodyType::Circle; }
	};

	class Sprite : public Body {
	public:
		Sprite() {}
		Sprite(int width, int height, ImageMemoryType memoryOrder);
		Sprite(void* pImageStream, size_t size);
		Sprite(const std::string& imageFile);

		Sprite(Sprite&& move) noexcept;
		Sprite(const Sprite& copy) noexcept;

		Sprite& operator=(const Sprite& copy) noexcept;
		Sprite& operator=(Sprite&& move) noexcept;

		~Sprite() noexcept;

		virtual BodyType GetType() override { return BodyType::Sprite; }

		void ReleaseMemory();

		void FlushMemory();
		void InvalidateMemory();

		void* GetBuffer();

		uint32_t Read(int x, int y);
		void Write(int x, int y, uint32_t value);
		
		inline void Write(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
			Write(x, y, (uint32_t(r) << 24) | (uint32_t(g) << 16) | (uint32_t(b) << 8) | a);
		}

		inline void Write(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
			Write(x, y, (uint32_t(r) << 16) | (uint32_t(g) << 8) | b);
		}

		inline void Write(int x, int y, uint8_t grayscale) {
			Write(x, y, grayscale);
		}

		inline void Write(int x, int y, float r, float g, float b, float a) {
			Write(x, y, uint8_t(r * 255.0f), uint8_t(g * 255.0f), uint8_t(b * 255.0f), uint8_t(a * 255.0f));
		}

		inline void Write(int x, int y, float r, float g, float b) {
			Write(x, y, uint8_t(r * 255.0f), uint8_t(g * 255.0f), uint8_t(b * 255.0f));
		}

		inline void Write(int x, int y, float grayscale) {
			Write(x, y, uint8_t(grayscale * 255.0f));
		}
	
		inline int Width() const { return m_width; }
		inline int Height() const { return m_height; }
		inline int Stride() const { return m_stride; }

		// void ToGrayscale();
		// void SaveToDisk() const;

	private:
		std::vector<uint8_t> m_data;
		int m_width = 0;
		int m_height = 0;
		int m_stride = 0;
		int m_comp_count = 1;
	};

	class Canvas {
	public:
		void Create(const egx::DeviceCtx& ctx, int width, int height, ImageMemoryType memoryType);
		void ReadToBuffer();
		void SaveScreenshotToDisk(const std::string& filepath);
	public:
		glm::ivec2 resolution;
		ImageMemoryType memoryType;
	private:
		friend class Scene;
		egx::IRenderTarget m_renderTarget;
	};

	struct SceneNode {
		bool isLeafNode;
		bool isBranchNode;
		glm::mat3 transform;
		std::shared_ptr<Body> body;
		std::unique_ptr<SceneNode> pChild;
		SceneNode* pParent;
		std::vector<SceneNode> pLeafs;
	};

	class Scene {
	public:
		void Init(const egx::DeviceCtx& ctx, const Canvas& canvas);
		void Paint();
	
	public:
		std::string name;
		Canvas canvas;
		SceneNode parentNode;
		std::list<std::shared_ptr<Body>> bodies;

	private:
		egx::DeviceCtx m_ctx;
		egx::Buffer m_drawCallData;
		egx::Buffer m_transformData;
		Canvas m_canvas;
		uint64_t m_lastTimeStamp;
		IGraphicsPipeline m_pipeline;
		egx::ResourceDescriptorPool m_shaderBindingPool;
		std::vector<egx::ResourceDescriptor> m_shaderBinding;
		std::vector<vk::CommandPool> m_pool;
		std::vector<vk::CommandBuffer> m_cmd;
		vk::Fence m_fence;
	};

	/*
		Scene anim;
		anim.name = "Pong Animation";
		RectBody ball1, ball2;
		ball1.SetSize(20, 250);
		ball2.SetSize(20, 250);
		ball1.AddTrajectory(dstx: 500, dsty: 10, motion: AnimTraj::Linear);
		ball2.AddTrajectory(dstx: 500, dsty: 490, motion: AnimTraj::Linear);
		ball1.SetColor(Color::Orange).SetOpacity(0.5f);
		ball2.SetColor(color::FromRGB(104, 22, 45)).SetOpacity(0.25f);
	*/

}
