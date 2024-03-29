#include <core/egx.hpp>
#include <pipeline/pipeline.hpp>
#include <pipeline/RenderTarget.hpp>
#include <pipeline/RenderGraph.hpp>
#include <pipeline/Sampler.hpp>
#include <window/BitmapWindow.hpp>
#include <window/swapchain.hpp>
#include <scene/IScene.hpp>
#include <ext/StopWatch.hpp>
#include <glm/vec4.hpp>
#include <numbers>

#include "graphics.hpp"
#include <scene/font/FontAtlas.hpp>
#include <stb/stb_image_write.h>

using namespace std;
using namespace egx;
using namespace glm;

int fps = 0;
float delta_time = 0;
vec2 mouse = { 0.0, 0.0 };
vec2 direction = { 0.0, 0.0 };
float direction_exponential_time = 0;
float shoot_angle = 90.0f;

struct {
	fvec2 position{ width / 2 - (block_size / 2), block_size };
} player;


struct particle {
	fvec2 position;
	fvec2 velocity;
	fvec2 acceleration;
	float acceleration_decay = 0;
	float life_time;
	int color;
	int radius;
};

list<particle> particles;
float emit_cooldown = 0;

bool up_key = false;
bool down_key = false;
bool left_key = false;
bool right_key = false;
bool space_key = false;
bool a_key = false;
bool d_key = false;
bool w_key = false;
bool s_key = false;

void render() {
	// clear screen buffers
	clear(0, 0);
	int c = rgb(140, 255, 188);

	for (int x = 0; x < width / block_size; x++) {
		fill_rect(block_to_pixel(x, 0), ivec2{ block_size }, rgb(50, 255, 50), 0);
	}
	draw_player(player.position, c, 255);

	if (left_key) {
		shoot_angle += +115.0f * delta_time;
	}
	if (right_key) {
		shoot_angle += -115.0f * delta_time;
	}
	float cc = 2.5f * block_size * delta_time;
	if (a_key) {
		player.position.x -= cc;
	}
	if (d_key) {
		player.position.x += cc;
	}
	if (w_key) {
		player.position.y += cc;
	}
	if (s_key) {
		player.position.y -= cc;
	}
	//player.position = glm::clamp(player.position, {}, { width, height });
	shoot_angle = glm::clamp(shoot_angle, 25.0f, 165.0f);

	if (!space_key) {
		emit_cooldown = delta_time;
	}

	const float emit_rate = 1.5e-3; // every 15 ms

	if (emit_cooldown < 0) {
		while (emit_cooldown < 0) {
			// emit particle
			particle p;
			p.position = { player.position.x + (block_size / 2), player.position.y + 2 * block_size };
			p.life_time = 2 + delta_time; // seconds
			p.velocity = { 2000, 2000 }; // pixels per second
			p.acceleration = { -100, -100 }; // deceleration pixels per second
			p.color = rand_color();
			p.radius = 5;
			p.velocity /= glm::max(p.radius / 2, 1);
			p.acceleration /= glm::max(p.radius / 2, 1);
			p.acceleration_decay = rand_float() * 2.0f;

			//vec2 direction = { cosf(radians(shoot_angle)), sinf(radians(shoot_angle)) };
			vec2 target_direction = normalize(vec2(mouse) - p.position);
			float lerp_factor = exp(-4.0f * direction_exponential_time);
			direction = (1.0f - lerp_factor) * target_direction + direction * lerp_factor;
			p.velocity *= direction;
			p.acceleration *= direction;
			direction_exponential_time += delta_time;

			particles.push_back(p);
			// reset cooldown
			emit_cooldown += emit_rate;
		}
		emit_cooldown = emit_rate;
	}
	else {
		emit_cooldown -= delta_time;
	}
	// remove expired particles
	for (auto i = particles.begin(); i != particles.end(); i++) {
		if (i->life_time <= 0 || i->position.y < 0 || i->position.y > height ||
			i->position.x < 0 || i->position.x > width) {
			i = particles.erase(i);
			if (i == particles.end())
				break;
		}
	}
	// render particles
	for (auto& p : particles) {
		p.life_time -= delta_time;
		p.position += p.velocity * delta_time;
		p.velocity += p.acceleration * delta_time;
		p.acceleration -= p.acceleration * p.acceleration_decay * delta_time;
		p.velocity = max(p.velocity, vec2(p.velocity.x, 5));
		fill_circle(p.position, p.radius, p.color, 1);
	}
}

int main() {
	shared_ptr<VulkanICDState> icd = VulkanICDState::Create("Application", false, false, VK_API_VERSION_1_3, nullptr, nullptr);
	DeviceCtx ctx = icd->CreateDevice(icd->QueryGPGPUDevices()[0]);

	PlatformWindow window("Application", width, height);
	ISwapchainController swapchain(ctx, &window);
	swapchain.SetResizeOnWindowResize(true).SetupImGui().SetVSync(true).Invalidate();
	//back_buffer = &window.GetBackBuffer();
	wrap_pixel_mode = false;

	Shader::SetGlobalCacheDirectory("./spirv");

	window.RegisterCallback(EVENT_FLAGS_MOUSE_MOVE, [](const Event& e, void*) {
		auto& p = e.mPayload;
		mouse = { p.mPositionX, p.mPositionY };
		direction_exponential_time = glm::max(direction_exponential_time - delta_time, 0.15f);
		});

	FontAtlas arial;
	arial.LoadTTFont("C:\\Windows\\Fonts\\Arial.ttf")
		.SetAtlasForMinimalMemoryUsage()
		.SetCharacterSet(L"qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890!@#$%^&*()-=_+[]\\{}|;':\",./<>?")
		.SetOptimalQuaility()
		.BuildAtlas(48, true, true);


	StopWatch frame_watch;
	while (!window.ShouldClose()) {
		frame_watch.Reset();
		PlatformWindow::Poll();
		swapchain.Acquire();

		swapchain.Present();
		frame_watch.Stop();
		fps = frame_watch.RatePerSecond();
		delta_time = frame_watch.TimeAsSeconds();
	}

	ctx->Device.waitIdle();

}
