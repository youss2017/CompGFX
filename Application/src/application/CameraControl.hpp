#pragma once
#include "../physics/PhysicsCore.hpp"
#include "window/PlatformWindow.hpp"
#include "Camera.hpp"
#include "pass/postprocess/GameUI.hpp"

class CameraControl {

public:
	CameraControl(PlatformWindow* window);
	~CameraControl();

	// GameUI Ptr
	void SetGameUI(void* UI);

	void LockCamera() { mLocked = true; }
	void UnlockCamera() { mLocked = false; }
	// make sure to load view matrix first
	glm::mat4 ReadLockedViewMatrix() {
		if (!mLocked) {
			memcpy(&mLockedCamera, &mCamera, sizeof(Camera));
		}
		return mLockedCamera.GetViewMatrix();
	}
	void LoadViewMatrix();
	glm::mat4 ReadViewMatrix() { return mCamera.GetViewMatrix(); }

public:
	bool mKeys[100];
	Camera mCamera;
	Camera mLockedCamera;

	struct {
		bool bSelect = false;
		glm::ivec2 iA{};
		glm::ivec2 iB{};
	} sMouseSelect;

private:
	int nCallbackIDs[3];
	PlatformWindow* mWindow;
	bool mLocked = false;
	Application::GameUI* mUI;

	double RotateRate = 45.0;

	Ph::Ray cursorRay = {};
	bool CameraMove = false;
	glm::vec2 Magnitude{};
	bool IOInit = true;

	glm::vec3 RayOrigin = glm::vec3(0.0);
	glm::vec3 RayDirection = glm::vec3(0.0, 0.0, 1.0f);

};