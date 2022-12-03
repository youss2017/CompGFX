#define CPP_UTILITY_IMPLEMENTATION
#include <Utility/CppUtility.hpp>
#include <egx/core/egx.hpp>
#include <memory>
using namespace egx;

int main()
{
	LOG(INFO, "SANDBOX");
	std::shared_ptr<EngineCore> engine = std::make_shared<EngineCore>(EngineCoreDebugFeatures::GPUAssisted, false);
	engine->EstablishDevice(engine->EnumerateDevices()[0]);
	auto& interface = engine->CoreInterface;
	auto sample = Shader(interface, "sample.vert");

	LOG(INFO, "Sample Reflection\n{0}", sample.ReflectionInfo());

}
