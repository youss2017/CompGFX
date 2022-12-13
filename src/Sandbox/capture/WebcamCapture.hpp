#pragma once
#include <string>
#include <vector>
#include <memory>

struct WebcamDeviceInformation {
	uint32_t Id;
	std::string Name;
	std::string Description;
	uint32_t AudioCaptureId;
};

struct WebcamOSCtx;

class WebcamCapture {
public:
	WebcamCapture(const WebcamDeviceInformation& information);
	~WebcamCapture();
	WebcamCapture(WebcamCapture&&) noexcept;
	WebcamCapture& operator=(WebcamCapture&&) noexcept;
	WebcamCapture(WebcamCapture&) = delete;

	bool Capture();

	static bool LoadCOM();
	static std::vector<WebcamDeviceInformation> EnumerateDevices();

	static void* GetWin32MonikerFromId(uint32_t id);

public:
	const WebcamDeviceInformation Information;

private:
	WebcamOSCtx* Ctx = nullptr;
};
