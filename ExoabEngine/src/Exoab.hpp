#pragma once
#include "Camera.hpp"
#include "core/memory/Textures.hpp"
#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <time.h>
#include <stb_image.h>
#include "graphics/Graphics.hpp"
#include "core/pipeline/Framebuffer.hpp"
#include "core/backend/GUI.h"
#include "graphics/material_system/MaterialConfiguration.hpp"
#include "graphics/material_system/FramebufferReserve.hpp"
#include "graphics/material_system/Material.hpp"
#include "mesh/Model.hpp"

struct Tristate {
	uint8_t state = 0xff; // 0 == off, 1 == enabled, 255 == disconnected
	void disable() { state = 0; }
	void enable() { state = 1; }
	void disconnect() { state = 0xff; }
	bool disabled()     { return state == 0; }
	bool enabled()      { return state == 1; }
	bool disconnected() { return state == 0xff; }

	static Tristate Enabled() {
		Tristate _;
		_.enable();
		return _;
	}

	static Tristate Disabled() {
		Tristate _;
		_.disable();
		return _;
	}

	static Tristate Disconnected() {
		Tristate _;
		_.disconnect();
		return _;
	}

};

bool Exoab_Initalize(ConfigurationSettings config);
void Exoab_Render(double dTimeFromStart, double dElapsedTime);
Tristate Exoab_Update(double dTimeFromStart, double dElapsedTime);
void Exoab_CleanUp();

void Exoab_GUI();
