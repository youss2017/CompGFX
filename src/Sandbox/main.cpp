#include <Utility/CppUtility.hpp>
#include <egx/core/egx.hpp>
#include <memory>
#include <random>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include "escapi.h"
#include <glm/glm.hpp>
#include <algorithm>

using namespace egx;

int GetWorkGroupCount(int size, int localSize) {
	return std::max((size + std::max<int>(size - localSize, 0)) / localSize, 1);
}

int main()
{
	LOG(INFO, "SANDBOX");

	auto engine = EngineCore(EngineCoreDebugFeatures::GPUAssisted, true);
	auto& options = engine.GetEngineLogger()->Options;
	options.ShowMessageBoxOnError = true;

	VkPhysicalDeviceFeatures2 features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

	auto& core = engine.EstablishDevice(engine.EnumerateDevices()[0], features);

	const char* comp = R"(
#version 450 core
layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

layout (binding = 0) buffer InDataBlock {
	float InData[];
};

layout (binding = 1) buffer OutDataBlock {
	float OutData[];
};

layout (push_constant) uniform pushblock {
	int DataSize;
};

void main() {
	if(gl_GlobalInvocationID.x < DataSize) {
		float x = InData[gl_GlobalInvocationID.x];
		float fout = 0.0;
		for(float i = 0; i < 10; i++) {
			fout += sin(x) / i;
		}
		fout = (fout + acos(x)) / 2.0;
		OutData[gl_GlobalInvocationID.x] = fout;
	}
}
)";

	uint32_t LocalSizeX = 32;
	auto compShader = Shader2::FactoryCreateEx(core, comp, VK_SHADER_STAGE_COMPUTE_BIT, egx::BindingAttributes::Default, "main.cpp", "Comp(x)");
	compShader->SetSpecializationConstant<uint32_t>(0, LocalSizeX);
	auto pipeline = Pipeline::FactoryCreate(core);
	pipeline->Create(compShader);

	std::vector<float> data(10000);
	for (int i = 0; i < data.size(); i++) data[i] = i;
	auto inputBuffer = Buffer::FactoryCreate(core, data.size() * sizeof(float), egx::memorylayout::dynamic, BufferType_Storage, false, false);
	auto outputBuffer = inputBuffer->Clone(false);

	inputBuffer->Write(data.data());

	pipeline->SetBuffer(0, 0, inputBuffer);
	pipeline->SetBuffer(0, 1, outputBuffer);

	auto cmd = CommandBufferSingleUse(core);
	pipeline->Bind(cmd.Cmd);
	int size = data.size();
	pipeline->PushConstants(cmd.Cmd, VK_SHADER_STAGE_COMPUTE_BIT, 0, 4, &size);
	vkCmdDispatch(cmd.Cmd, GetWorkGroupCount(size, LocalSizeX), 1, 1);
	cmd.Execute();

	outputBuffer->Read(&data[0]);
}
