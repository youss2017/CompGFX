#include <iostream>
#include <Utility/CppUtility.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <numbers>
#include <random>
#include <stb/stb_image_write.h>
#include <core/egx.hpp>
#include <window/swapchain.hpp>
#include <pipeline/pipeline.hpp>
#include <pipeline/RenderGraph.hpp>
#include <pipeline/Sampler.hpp>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>
#include <filesystem>
#include <execution>

using namespace std;
using namespace glm;

namespace param {
	constexpr auto Charge = 1.60217663e-9f;
	constexpr auto PermitivityOfFreeSpace = 8.85418782e-12f;
	constexpr float ColoumbConstant = 1.0f / (4.0f * numbers::pi * PermitivityOfFreeSpace);
	float TimeStep = 0.001f;
	constexpr vec2 ViewBoxMin = { -80.0, -80.0 };
	constexpr vec2 ViewBoxMax = { +80.0, +80.0 };
	constexpr ivec2 OutputImageSize = { 650, 650 };
	constexpr int LocalSizeX = 32;
	constexpr int LocalSizeY = 32;
	constexpr int ParticleCount = 10;
}
vector<vec4> particles;
vector<vec4> particle_buffer;
double max_magnitude = numeric_limits<double>::min();

struct Particle {
	vec3 position = {};
	vec3 velocity = {};
	vec3 acceleration = {};
	float charge = 1;
	int stationary = 0;
};

float mapRange(float val, float in_min, float in_max,
	float out_min, float out_max) {
	return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

vector<Particle> GenerateParticles(size_t seed, size_t count, int constant_charge = 0) {
	mt19937 rd(seed);
	uniform_real_distribution<double> dist_x(-15.0, 15.0);
	uniform_real_distribution<double> dist_y(-15.0, 15.0);
	uniform_int_distribution charge(0, 1);
	vector<Particle> particles(count);
	//particles.resize(count), particle_buffer.resize(count);
	for (auto i = 0; i < count; i++) {
		particles[i].position = {
				dist_x(rd),
				dist_y(rd),
				0.0, //dist(rd)
		};
		particles[i].charge = constant_charge == 0 ? (charge(rd) == 1 ? +1.0f : -1.0f) : constant_charge;
		particles[i].stationary = charge(rd);
	}
	return particles;
}

int main()
{
	using namespace egx;
	auto icd = VulkanICDState::Create("Sandbox", true, true, VK_API_VERSION_1_3, nullptr, nullptr);
	auto gpu_info = icd->QueryGPGPUDevices()[0];
	vk::PhysicalDeviceSynchronization2FeaturesKHR sync_feature;
	sync_feature.setSynchronization2(true);
	gpu_info.EnabledFeatures.pNext = &sync_feature;
	auto ctx = icd->CreateDevice(gpu_info);
	ctx->FramesInFlight = 3;
	PlatformWindow window("Sandbox", 1920, 1080);
	SwapchainController swapchain(ctx, &window);
	swapchain.SetResizeOnWindowResize(false).Invalidate();

	auto arial_ttf_file = cpp::ReadAllBytes("Orbitron-Regular.ttf");

	stbtt_fontinfo font;
	stbtt_InitFont(&font, arial_ttf_file->data(), stbtt_GetFontOffsetForIndex(arial_ttf_file->data(), 0));

	auto GetSdfCodepointBitmap = [](stbtt_fontinfo* font, int fontSize, wchar_t ch, int* pWidth, int* pHeight, int targetResolution = 0) -> vector<uint8_t> {
		// develop sdf
		int upscale_resolution;
		if (targetResolution == 0) {
			upscale_resolution = std::min(fontSize << 3, 2048);
			if (fontSize > upscale_resolution) {
				upscale_resolution = fontSize;
			}
		}
		else {
			upscale_resolution = targetResolution;
		}
		const int spread = upscale_resolution / 2;
		int up_w, up_h;
		auto upscale_bitmap = stbtt_GetCodepointBitmap(font, 0, stbtt_ScaleForPixelHeight(font, upscale_resolution), ch, &up_w, &up_h, 0, 0);

		float widthScale = up_w / (float)upscale_resolution;
		float heightScale = up_h / (float)upscale_resolution;
		int characterWidth = fontSize * widthScale;
		int characterHeight = fontSize * heightScale;
		float bitmapScaleX = up_w / (float)characterWidth;
		float bitmapScaleY = up_h / (float)characterHeight;
		vector<uint8_t> sdf_bitmap(characterWidth * characterHeight);

		for (int y = 0; y < characterHeight; y++) {
			for (int x = 0; x < characterWidth; x++) {
				// map from [0, characterWidth] (font size scale) to [0, up_w]
				int pixelX = (x / (float)characterWidth) * up_w;
				int pixelY = (y / (float)characterHeight) * up_h;
				// find nearest pixel
				auto read_pixel = [](uint8_t* bitmap, int x, int y, int width, int height) -> bool {
					if (x < 0 || x >= width || y < 0 || y >= height) return false;
					uint8_t value = bitmap[y * width + x];
					return value & 0xFF;
				};

				int minX = pixelX - spread;
				int maxX = pixelX + spread;
				int minY = pixelY - spread;
				int maxY = pixelY + spread;
				float minDistance = spread * spread;

				for (int yy = minY; yy < maxY; yy++) {
					for (int xx = minX; xx < maxX; xx++) {
						bool pixelState = read_pixel(upscale_bitmap, xx, yy, up_w, up_h);
						if (pixelState) {
							float dxSquared = (xx - pixelX) * (xx - pixelX);
							float dySquared = (yy - pixelY) * (yy - pixelY);
							float distanceSquared = dxSquared + dySquared;
							minDistance = std::min(minDistance, distanceSquared);
						}
					}
				}

				minDistance = sqrtf(minDistance);
				bool state = read_pixel(upscale_bitmap, pixelX, pixelY, up_w, up_h);
				float output = (minDistance - 0.5f) / (spread - 0.5f);
				output *= state == 0 ? -1 : 1;
				// Map from [-1, 1] to [1, 1]
				output = (output + 1.0f) * 0.5f;

				// store pixel
				sdf_bitmap[y * characterWidth + x] = output * 255.0f;
			}
		}
		*pWidth = characterWidth;
		*pHeight = characterHeight;
		::free(upscale_bitmap);
		return sdf_bitmap;
	};

	wstring character_set = L"1~!@#$%^&*()_+`1234567890-=QWERTYUIOP{}qwertyuiop[]\\|asdfghjkl;'ASDFGHJKL:\"zxcvbnm,m./ZXCVBNM<>?";
	vector<tuple<wchar_t, int, int, vector<uint8_t>>> unordered_bitmaps;
	vector<tuple<wchar_t, int, int, vector<uint8_t>>> bitmaps;
	int total_pixel_area = 0;
	mutex m;
	for_each(execution::par_unseq, character_set.begin(), character_set.end(), [&](wchar_t ch) {
		int w, h;
		auto character_bitmap = GetSdfCodepointBitmap(&font, 50, ch, &w, &h);
		m.lock();
		unordered_bitmaps.push_back({ ch, w, h, move(character_bitmap) });
		total_pixel_area += w * h;
		m.unlock();
		});

	for (auto& ch : character_set) {
		auto item = find_if(unordered_bitmaps.begin(), unordered_bitmaps.end(), [ch](tuple<wchar_t, int, int, vector<uint8_t>>& e) { return ch == get<0>(e); });
		auto& [_, w, h, bp] = *item;
		bitmaps.push_back({ ch, w, h, move(bp) });
		unordered_bitmaps.erase(item);
	}
	bitmaps.shrink_to_fit();

	// 1) determine bitmap resolution
	int dimension = 1 << 8;
	while (dimension * dimension <= total_pixel_area) dimension <<= 1;
	dimension <<= 1;

	struct character_info {
		wchar_t ch;
		int start_x;
		int start_y;
		int width;
		int height;
	};

	vector<character_info> character_map;
	character_map.reserve(character_set.size());
	vector<uint8_t> font_bitmap(dimension * dimension);
	int cursor_x = 0;
	int cursor_y = 0;
	int line_max_height = 0;
	int line_character_count = 0;
	int font_map_height = 0;
	for (auto& [ch, w, h, ch_bitmap] : bitmaps) {
		character_info cinfo{};
		cinfo.ch = ch, cinfo.width = w, cinfo.height = h;
		if (dimension - cursor_x >= w) {
			// add character at end of line
			cinfo.start_x = cursor_x;
			cinfo.start_y = cursor_y;
			line_max_height = std::max(h, line_max_height);
			line_character_count++;
		}
		else {
			// go to next line
			line_character_count = 0;
			cursor_x = 0;
			cursor_y += line_max_height;
			line_max_height = 0;
			// add character
			cinfo.start_x = cursor_x;
			cinfo.start_y = cursor_y;
		}

		// optimize cursor_y for character
		if (cursor_y > 0) {
			// loop through all character that are behind this character
			vector<character_info> overlapping_characters;
			overlapping_characters.reserve(5);
			for (int i = character_map.size() - 1 - line_character_count; i >= 0; i--) {
				const auto& previous_character = character_map[i];
				int x_min = cinfo.start_x;
				int x_max = x_min + cinfo.width;
				int px_min = previous_character.start_x;
				int px_max = px_min + previous_character.width;
				if (/* is x_min inside [px_min, px_max] */
					(x_min >= px_min && x_min < px_max) ||
					/* is x_max inside [px_min, px_max] */
					(x_max >= px_min && x_max < px_max)) {
					overlapping_characters.push_back(previous_character);
				}
				if (px_min == 0) {
					// we are at the begining of the previous line
					// therefore we can break
					break;
				}
			}
			// adjust start_y from the maximum height of all overlappying characters
			int temp = cinfo.start_y;
			cinfo.start_y = 0;
			for (auto& overlapping_ch : overlapping_characters) {
				int start_y = overlapping_ch.start_y + overlapping_ch.height;
				cinfo.start_y = std::max(cinfo.start_y, start_y);
			}
		}
		// copy character bitmap to font bitmap
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				uint8_t value = ch_bitmap[y * w + x];
				size_t index = dimension * (cinfo.start_y + y) + (cinfo.start_x + x);
				font_bitmap[index] = value;
			}
		}
		font_map_height = std::max(font_map_height, cinfo.start_y + cinfo.height);
		cursor_x += w;
		character_map.push_back(cinfo);
		ch_bitmap.clear();
	}

	// shrink font map
	{
		int reduced_height = dimension;
		while (font_map_height < reduced_height) {
			reduced_height >>= 1;
			if (reduced_height < font_map_height) {
				reduced_height <<= 1;
				break;
			}
		}
		font_map_height = reduced_height;
		font_bitmap.resize(/* width */ dimension * /* height */ font_map_height);
	}

	stbi_write_bmp("font_bitmap.bmp", dimension, font_map_height, 1, font_bitmap.data());
	if (!filesystem::exists("font_output"))
		filesystem::create_directory("font_output");

	for (auto& chinfo : character_map) {
		char ch = (char)chinfo.ch;
		string file_name = cpp::Format("font_output/{}.bmp", ch);
		try {
			vector<uint8_t> bitmap(chinfo.width * chinfo.height);
			for (int y = 0; y < chinfo.height; y++) {
				for (int x = 0; x < chinfo.width; x++) {
					bitmap[y * chinfo.width + x] = font_bitmap[(chinfo.start_y + y) * dimension + (chinfo.start_x + x)];
				}
			}
			stbi_write_bmp(file_name.c_str(), chinfo.width, chinfo.height, 1, bitmap.data());
		}
		catch (...) {}
	}

	IRenderTarget rt(ctx, swapchain, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		vk::ClearValue().setColor({ 0.0f, 0.0f, 0.2f, 1.0f }));
	rt.EnableDearImGui(window).Invalidate();

	ComputePipeline simulation_pipeline(ctx, Shader(ctx, "simulation.comp"));

	size_t seed = random_device()();
	auto particles = GenerateParticles(seed, param::ParticleCount);

	Buffer srcParticles(ctx, particles.size() * sizeof(Particle), MemoryPreset::DeviceAndHost, HostMemoryAccess::Default, vk::BufferUsageFlagBits::eStorageBuffer, false);
	Buffer dstParticles(ctx, particles.size() * sizeof(Particle), MemoryPreset::DeviceAndHost, HostMemoryAccess::Default, vk::BufferUsageFlagBits::eStorageBuffer, false);

	srcParticles.Write(particles.data());

	Image2D output(ctx, param::OutputImageSize, vk::Format::eR8G8B8A8Unorm, 1, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eShaderReadOnlyOptimal, false);
	output.CreateView(0);
	ISamplerBuilder defaultSampler(ctx);
	defaultSampler.Invalidate();

	RenderGraph graph(ctx);
	auto resources = graph.CreateResourceDescriptor(simulation_pipeline);
	resources.SetInput(0, srcParticles);
	resources.SetInput(1, dstParticles);
	resources.SetInput(2, vk::ImageLayout::eGeneral, 0, output);

	struct pushblock {
		float time_step = param::TimeStep;
		float z_slice = 0.0;
		float coloumb_constant = param::ColoumbConstant;
		float charge = param::Charge;
		vec2 view_box_min = param::ViewBoxMin;
		vec2 view_box_max = param::ViewBoxMax;
		int store_result = 1;
		int show_directional_ef = 0;
	};

	pushblock parameters{};
	int counter = 0;
	float total_time = 0.0;
	bool new_seed = false;
	bool constant_charge = false;

	auto func = [&](vk::CommandBuffer cmd) {
		rt.BeginDearImguiFrame();

		resources.Bind(cmd, simulation_pipeline.BindPoint(), simulation_pipeline.Layout());
		//ImGui::ShowDemoWindow();

		ImGui::Begin("Simulation");
		ImGui::Columns(2);
		ImGui::Image(output.GetImGuiTextureID(defaultSampler), { param::OutputImageSize.x, param::OutputImageSize.y });
		ImGui::NextColumn();
		if (counter <= 1) {
			if (ImGui::Button("Simulate")) {
				counter = 1;
			}
			ImGui::SameLine();
			if (ImGui::Button("Run")) {
				counter = numeric_limits<int>::max();
			}
		}
		else {
			ImGui::BeginDisabled();
			ImGui::Button("Simulate");
			ImGui::SameLine();
			ImGui::EndDisabled();
			if (ImGui::Button("Stop")) {
				counter = 0;
			}
		}
		ImGui::SameLine();
		ImGui::Text("Total Time %.6f sec / Time Step %.6f sec | seed = 0x%X", total_time, param::TimeStep, seed);

		ImGui::InputFloat("Time Step", &param::TimeStep, 0, 0, "%.3f", ImGuiInputTextFlags_CharsScientific);

		parameters.store_result = 1;
		if (parameters.show_directional_ef == 0) {
			if (ImGui::Button("Show Electric Field Direction")) {
				parameters.show_directional_ef = 1, parameters.store_result = 0;
				counter = std::max(counter, 1);
			}
		}
		else {
			if (ImGui::Button("Show Electric Field Strength")) {
				parameters.show_directional_ef = 0, parameters.store_result = 0;
				counter = std::max(counter, 1);
			}
		}

		bool reset;
		reset = ImGui::Button("Reset");
		ImGui::SameLine(), ImGui::Checkbox("New Seed?", &new_seed);
		ImGui::SameLine(), ImGui::Checkbox("Constant Charge", &constant_charge);

		if (reset) {
			if (new_seed) {
				seed = random_device()();
			}
			auto particles = GenerateParticles(seed, param::ParticleCount, constant_charge ? -1 : 0);
			srcParticles.Write(particles.data());
			total_time = 0.0;
		}

		srcParticles.Read(particles.data());

		if (ImGui::BeginTable("Particle Info", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_NoSavedSettings |
			ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY)) {
			ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
			ImGui::TableSetColumnIndex(0), ImGui::Text("Id");
			ImGui::TableSetColumnIndex(1), ImGui::Text("Position");
			ImGui::TableSetColumnIndex(2), ImGui::Text("Velocity");
			ImGui::TableSetColumnIndex(3), ImGui::Text("Acceleration");
			ImGui::TableSetColumnIndex(4), ImGui::Text("Charge");

			for (uint32_t i = 0; i < particles.size(); i++) {
				ImGui::TableNextRow();
				// counter
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%d", i);
				// position
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("(%.2f, %.2f, %.2f)", particles[i].position.x, particles[i].position.y, particles[i].position.z);
				// velocity
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("(%.2f, %.2f, %.2f)", particles[i].velocity.x, particles[i].velocity.y, particles[i].velocity.z);
				// acceleration
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("(%.2f, %.2f, %.2f)", particles[i].acceleration.x, particles[i].acceleration.y, particles[i].acceleration.z);
				// Charge
				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%s", particles[i].charge == 1 ? "+1 Proton" : "-1 Electron");
			}
			ImGui::EndTable();
		}

		ImGui::End();

		if (counter-- > 0) {
			vk::ImageMemoryBarrier2 compute_shader_image_barrier;
			compute_shader_image_barrier.setSrcAccessMask(vk::AccessFlagBits2::eNone)
				.setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
				.setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setNewLayout(vk::ImageLayout::eGeneral)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setSubresourceRange(vk::ImageSubresourceRange().setAspectMask(vk::ImageAspectFlagBits::eColor).setLayerCount(VK_REMAINING_ARRAY_LAYERS).setLevelCount(VK_REMAINING_MIP_LEVELS))
				.setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
				.setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
				.setImage(output.GetHandle());

			vk::DependencyInfo depInfo;
			depInfo.setImageMemoryBarriers(compute_shader_image_barrier);
			cmd.pipelineBarrier2(depInfo);

			int group_count_x = param::OutputImageSize.x / param::LocalSizeX;
			int group_count_y = param::OutputImageSize.y / param::LocalSizeY;
			if (param::OutputImageSize.x % param::LocalSizeX != 0) group_count_x++;
			if (param::OutputImageSize.y % param::LocalSizeY != 0) group_count_y++;

			cmd.pushConstants(simulation_pipeline.Layout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pushblock), &parameters);
			cmd.dispatch(group_count_x, group_count_y, 1);

			compute_shader_image_barrier.setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
				.setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
				.setOldLayout(vk::ImageLayout::eGeneral)
				.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setSubresourceRange(vk::ImageSubresourceRange().setAspectMask(vk::ImageAspectFlagBits::eColor).setLayerCount(VK_REMAINING_ARRAY_LAYERS).setLevelCount(VK_REMAINING_MIP_LEVELS))
				.setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
				.setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
				.setImage(output.GetHandle());

			vk::BufferMemoryBarrier2 compute_shader_buffer_barrier;
			compute_shader_buffer_barrier
				.setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
				.setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setSize(VK_WHOLE_SIZE)
				.setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
				.setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
				.setBuffer(dstParticles.GetHandle());

			depInfo.setImageMemoryBarriers(compute_shader_image_barrier).setBufferMemoryBarriers(compute_shader_buffer_barrier);
			cmd.pipelineBarrier2(depInfo);
			dstParticles.CopyTo(cmd, srcParticles);

			total_time += param::TimeStep;
		}

		rt.Begin(cmd);
		rt.EndDearImguiFrame(cmd);
		rt.End(cmd);
	};

	graph.Add(simulation_pipeline, func);

	while (!window.ShouldClose()) {
		PlatformWindow::Poll();
		swapchain.AcquireFullLock();
		graph.Run();
		swapchain.Present();
	}

	ctx->Device.waitIdle();
}