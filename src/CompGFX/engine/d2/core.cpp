#include "core.hpp"
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <Utility/CppUtility.hpp>

using namespace egx::d2;
using namespace std;
using namespace glm;

#pragma region "Color"
Color egx::d2::Color::FromRGB(uint8_t red, uint8_t green, uint8_t blue)
{
	return Color(red, green, blue, 255);  // Assuming 255 as the default alpha value
}

Color egx::d2::Color::FromRGB(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
	return Color(red, green, blue, alpha);
}

Color egx::d2::Color::FromRGB(float red, float green, float blue)
{
	// Assuming that the Color class has a constructor that takes floats and converts to uint8_t
	return Color(static_cast<uint8_t>(red * 255), static_cast<uint8_t>(green * 255), static_cast<uint8_t>(blue * 255), 255);
}

Color egx::d2::Color::FromRGB(float red, float green, float blue, float alpha)
{
	return Color(static_cast<uint8_t>(red * 255), static_cast<uint8_t>(green * 255), static_cast<uint8_t>(blue * 255), static_cast<uint8_t>(alpha * 255));
}

uint32_t egx::d2::Color::ToInt32()
{
	// Assuming a 32-bit integer where each 8 bits represent RGBA
	return (static_cast<uint32_t>(rgba[3]) << 24) | (static_cast<uint32_t>(rgba[0]) << 16) | (static_cast<uint32_t>(rgba[1]) << 8) | static_cast<uint32_t>(rgba[2]);
}

glm::vec3 egx::d2::Color::ToVec3()
{
	return {
		float(rgba[0]) / 255.0f,
		float(rgba[1]) / 255.0f,
		float(rgba[2]) / 255.0f
	};
}

Color egx::d2::Color::Random()
{
	return Color(rand() % 255, rand() % 255, rand() % 255);
}

// Define the static color variables
Color Color::Black = Color(0, 0, 0);
Color Color::Pink = Color(255, 192, 203);
Color Color::Orange = Color(255, 165, 0);
Color Color::Blue = Color(0, 0, 255);
Color Color::Red = Color(255, 0, 0);
Color Color::Yellow = Color(255, 255, 0);
Color Color::Green = Color(0, 128, 0);
Color Color::White = Color(255, 255, 255);
Color Color::Brown = Color(165, 42, 42);
Color Color::Silver = Color(192, 192, 192);
Color Color::Teal = Color(0, 128, 128);
Color Color::Violet = Color(238, 130, 238);
Color Color::Purple = Color(128, 0, 128);
Color Color::Magenta = Color(255, 0, 255);
Color Color::Cyan = Color(0, 255, 255);
Color Color::Grey = Color(128, 128, 128);
Color Color::Olive = Color(128, 128, 0);
Color Color::NavyBlue = Color(0, 0, 128);
#pragma endregion "Color"

uint32_t egx::d2::Sprite::Read(int x, int y)
{
	return 0;
}

void egx::d2::Sprite::Write(int x, int y, uint32_t value)
{
	int idx = y * m_stride + (x * m_comp_count);
	if (idx + m_comp_count >= m_data.size()) {
#ifdef _DEBUG
		LOG(WARNING, "Attempting to write outside of Sprite at [{}, {}]; write has been discarded.", x, y);
#endif
		return;
	}
	switch (m_comp_count) {
	case 4:
		m_data[idx] = (value >> 24);
		m_data[idx + 1] = (value >> 16) & 0xff;
		m_data[idx + 2] = (value >> 8) & 0xff;
		m_data[idx + 3] = (value) & 0xff;
		break;
	case 3:
		m_data[idx + 1] = (value >> 16) & 0xff;
		m_data[idx + 2] = (value >> 8) & 0xff;
		m_data[idx + 3] = (value) & 0xff;
		break;
	default:
		m_data[idx] = uint8_t(value & 0xff);
	}
}

egx::d2::Sprite::Sprite(int width, int height, ImageMemoryType memoryOrder)
{
	int comp_count = 4;
	switch (memoryOrder) {
	case ImageMemoryType::GRAYSCALE:
		comp_count = 1;
		break;
	case ImageMemoryType::RGBA:
		comp_count = 4;
		break;
	case ImageMemoryType::RGB:
		comp_count = 3;
		break;
	default:
		throw std::exception("Unknown image memory type.");
	}
	m_data.resize(static_cast<size_t>(width) * height * comp_count);
	m_width = width;
	m_height = height;
	m_stride = width * comp_count;
	m_comp_count = comp_count;
}

egx::d2::Sprite::Sprite(void* pImageStream, size_t size)
{
	int comp_count = 0;
	stbi_uc* memory = stbi_load_from_memory((stbi_uc*)pImageStream, static_cast<int>(size), &m_width, &m_height, &comp_count, 0);
	if (!memory) {
		throw std::exception("Could not parse memory stream.");
	}
	m_data.resize(static_cast<size_t>(m_width) * m_height * comp_count);
	m_stride = m_width * comp_count;
	m_comp_count = comp_count;
	memcpy(m_data.data(), memory, static_cast<size_t>(m_stride) * m_height);
	stbi_image_free(memory);
}

egx::d2::Sprite::Sprite(const std::string& imageFile)
{
	int comp_count = 0;
	stbi_uc* memory = stbi_load(imageFile.c_str(), &m_width, &m_height, &comp_count, 0);
	if (!memory) {
		throw std::exception("Could not parse memory stream.");
	}
	m_data.resize(static_cast<size_t>(m_width) * m_height * comp_count);
	m_stride = m_width * comp_count;
	m_comp_count = comp_count;
	memcpy(m_data.data(), memory, static_cast<size_t>(m_stride) * m_height);
	stbi_image_free(memory);
}

egx::d2::Sprite::Sprite(Sprite&& move) noexcept
{
	m_width = exchange(move.m_width, 0);
	m_height = exchange(move.m_height, 0);
	m_stride = exchange(move.m_stride, 0);
	m_comp_count = exchange(move.m_comp_count, 0);
	m_data = std::move(move.m_data);
}

egx::d2::Sprite::Sprite(const Sprite& copy) noexcept
{
	m_width = copy.m_width;
	m_height = copy.m_height;
	m_stride = copy.m_stride;
	m_comp_count = copy.m_comp_count;
	m_data = copy.m_data;
}

Sprite& egx::d2::Sprite::operator=(const Sprite& copy) noexcept
{
	m_width = copy.m_width;
	m_height = copy.m_height;
	m_stride = copy.m_stride;
	m_comp_count = copy.m_comp_count;
	m_data = copy.m_data;
	return *this;
}

Sprite& egx::d2::Sprite::operator=(Sprite&& move) noexcept
{
	m_width = exchange(move.m_width, 0);
	m_height = exchange(move.m_height, 0);
	m_stride = exchange(move.m_stride, 0);
	m_comp_count = exchange(move.m_comp_count, 0);
	m_data = std::move(move.m_data);
	return *this;
}

egx::d2::Sprite::~Sprite() noexcept
{
	// do nothing for now.
	return;
}

void egx::d2::Sprite::ReleaseMemory()
{
	// do nothing for now.
	return;
}

void egx::d2::Sprite::FlushMemory()
{
	// do nothing for now.
	return;
}

void egx::d2::Sprite::InvalidateMemory()
{
	// do nothing for now.
	return;
}

void* egx::d2::Sprite::GetBuffer()
{
	// TODO: update for gpu buffer impl
	return m_data.data();
}

void egx::d2::Body::NextAnimation(float deltaTime)
{
	velocity += acceleration * deltaTime;
	position += velocity * deltaTime;
	lifetime -= deltaTime;
}

void egx::d2::Canvas::Create(const egx::DeviceCtx& ctx, int width, int height, ImageMemoryType memoryType)
{
	m_renderTarget = egx::IRenderTarget(ctx, width, height);
	vk::Format format = vk::Format::eR8G8B8A8Unorm;
	switch (memoryType) {
	case ImageMemoryType::RGBA:
		format = vk::Format::eR8G8B8A8Unorm;
		break;
	case ImageMemoryType::RGB:
	case ImageMemoryType::GRAYSCALE:
		format = vk::Format::eR8G8B8Unorm;
		break; 
	default:
		throw std::runtime_error("Invalid memory type.");
	}
	m_renderTarget.CreateColorAttachment(0, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
		vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore);
	m_renderTarget.Invalidate();
}

void egx::d2::Canvas::SaveScreenshotToDisk(const std::string& filepath)
{
	const auto w = m_renderTarget.Width();
	const auto h = m_renderTarget.Height();
	vector<uint8_t> buffer(w * h * 4);
	m_renderTarget.GetAttachment(0).Read(0, buffer.data());
	stbi_write_png(filepath.c_str(), w, h, 4, buffer.data(), w * 4);
}

void egx::d2::Scene::Init(const egx::DeviceCtx& ctx, const Canvas& canvas)
{
	m_ctx = ctx;
	m_canvas = canvas;
	m_drawCallData = Buffer(ctx, 1024 * 1024, egx::MemoryPreset::DeviceAndHost, egx::HostMemoryAccess::Sequential, vk::BufferUsageFlagBits::eStorageBuffer, true);
	m_transformData = Buffer(ctx, 100 * sizeof(glm::mat4), egx::MemoryPreset::DeviceAndHost, egx::HostMemoryAccess::Sequential, vk::BufferUsageFlagBits::eStorageBuffer, true);
	Shader vertex(ctx, "C:\\Users\\youssef\\source\\repos\\CompGFX\\src\\CompGFX\\internal_assets\\d2\\body_vertex_shader.vert");
	Shader fragment(ctx, "C:\\Users\\youssef\\source\\repos\\CompGFX\\src\\CompGFX\\internal_assets\\d2\\body_fragment_shader.frag");
	PipelineSpecification spec;
	spec
		.SetFrontFace(vk::FrontFace::eClockwise).SetCullMode(vk::CullModeFlagBits::eNone)
		.SetDepthEnabled(false, false)
		.SetDepthCompare(vk::CompareOp::eLess);
	m_pipeline = IGraphicsPipeline(ctx, vertex, fragment, canvas.m_renderTarget, spec);
	m_pipeline.Invalidate();
	m_shaderBindingPool = egx::ResourceDescriptorPool(ctx);
	
	for (auto i = 0u; i < ctx->FramesInFlight; i++) {
		m_pool.push_back(ctx->Device.createCommandPool({}));
		m_cmd.push_back(m_ctx->Device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandBufferCount(1)
			.setCommandPool(m_pool[i]))[0]);
		m_shaderBinding.push_back(egx::ResourceDescriptor(ctx, m_shaderBindingPool, m_pipeline));
	}
	m_fence = m_ctx->Device.createFence({});
}

void egx::d2::Scene::Paint()
{
	// 1) Upload nodes into gpu buffer
	size_t vertices_count = bodies.size();
	size_t vertices_size = vertices_count * sizeof(DrawCall);
	if (m_drawCallData.Size() < vertices_size) {
		m_drawCallData.Resize(vertices_size);
	}
	// (TODO): Make minimim buffer size
	else if (m_drawCallData.Size() > vertices_size * 2) {
		m_drawCallData.Resize(vertices_size);
	}
	uint8_t* vt_ptr = (uint8_t*)m_drawCallData.Map();
	uint8_t* tf_ptr = (uint8_t*)m_transformData.Map();
	size_t vt_offset = 0;
	size_t tf_offset = 0;
	for (const auto& body : bodies) {
		DrawCall b = {
			.position = {body->position, float(body->zindex)},
			.flag = 0,
			.color_or_uv = body->color.ToVec3(),
			.opacity = body->opacity
		};
		// (TODO): Get object actual transform
		mat4 dummy{ 1.0 };
		memcpy(vt_ptr + vt_offset, &b, sizeof(DrawCall));
		memcpy(tf_ptr + tf_offset, &dummy, sizeof(mat4));
		vt_offset += sizeof(DrawCall);
		tf_offset += sizeof(mat4);
	}
	m_drawCallData.FlushToGpu();
	m_transformData.FlushToGpu();
	// 2) Instance Draw Call
	auto fidx = m_ctx->CurrentFrame;
	// update shader binding
	m_shaderBinding[fidx].SetInput(0, m_drawCallData);
	m_shaderBinding[fidx].SetInput(1, m_transformData);
	m_ctx->Device.resetCommandPool(m_pool[fidx]);
	m_cmd[fidx].begin(vk::CommandBufferBeginInfo());
	m_canvas.m_renderTarget.Begin(m_cmd[fidx]);
	m_cmd[fidx].bindPipeline(m_pipeline.BindPoint(), m_pipeline.Pipeline());
	//m_cmd[fidx].bindVertexBuffers(0, m_drawCallData.GetHandle(), {0});
	m_shaderBinding[fidx].Bind(m_cmd[fidx]);
	m_cmd[fidx].draw(6, (uint32_t)bodies.size(), 0, 0);
	m_canvas.m_renderTarget.End(m_cmd[fidx]);
	m_cmd[fidx].end();
	vk::SubmitInfo submit;
	submit.setCommandBufferCount(1).setCommandBuffers(m_cmd[fidx]);
	m_ctx->Queue.submit(submit, m_fence);
	m_ctx->Device.waitForFences(m_fence, true, 1e9);
	m_ctx->Device.resetFences(m_fence);
}
