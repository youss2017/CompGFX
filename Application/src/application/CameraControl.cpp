#include "CameraControl.hpp"
#include "Globals.hpp"
#include "UI.hpp"

constexpr int W_Key = 0;
constexpr int S_Key = 1;
constexpr int A_Key = 2;
constexpr int D_Key = 3;
constexpr int Q_Key = 4;
constexpr int E_Key = 5;
constexpr int Z_Key = 6;
constexpr int X_Key = 7;
constexpr int UP_Key = 8;
constexpr int DOWN_Key = 9;

CameraControl::CameraControl(PlatformWindow* window) : mWindow(window), mUI(nullptr) {
	memset(mKeys, 0, sizeof(mKeys));
	memset(nCallbackIDs, -1, sizeof(nCallbackIDs) / sizeof(int));
	nCallbackIDs[0] = mWindow->RegisterCallback(EVENT_KEY_PRESS | EVENT_KEY_RELEASE, [&](const Event& e, void* pUserDefined) throw() -> void {
		switch (e.mPayload.KeyLowerCase) {
		case 'w':
			mKeys[W_Key] = e.mEvents & EVENT_KEY_PRESS;
			break;
		case 's':
			mKeys[S_Key] = e.mEvents & EVENT_KEY_PRESS;
			break;
		case 'a':
			mKeys[A_Key] = e.mEvents & EVENT_KEY_PRESS;
			break;
		case 'd':
			mKeys[D_Key] = e.mEvents & EVENT_KEY_PRESS;
			break;
		case 'q':
			mKeys[Q_Key] = e.mEvents & EVENT_KEY_PRESS;
			break;
		case 'e':
			mKeys[E_Key] = e.mEvents & EVENT_KEY_PRESS;
			break;
		case 'z':
			mKeys[Z_Key] = e.mEvents & EVENT_KEY_PRESS;
			break;
		case 'x':
			mKeys[X_Key] = e.mEvents & EVENT_KEY_PRESS;
			break;
		};
		switch (e.mPayload.NonASCIKey) {
		case GLFW_KEY_UP:
			mKeys[UP_Key] = e.mEvents & EVENT_KEY_PRESS;
			break;
		case GLFW_KEY_DOWN:
			mKeys[DOWN_Key] = e.mEvents & EVENT_KEY_PRESS;
			break;
		}
		}
	, nullptr);
}

CameraControl::~CameraControl()
{
	for (int i = 0; i < sizeof(nCallbackIDs) / sizeof(int); i++)
		if (nCallbackIDs[i] != -1)
			mWindow->RemoveCallback(nCallbackIDs[i]);
}

void CameraControl::SetGameUI(void* UI) {
	mUI = (Application::GameUI*)UI;
	nCallbackIDs[1] = mWindow->RegisterCallback(EVENT_MOUSE_PRESS | EVENT_MOUSE_RELEASE, [&](const Event& e, void* pUserDefined) throw() -> void {
		using namespace glm;
		if (e.mEvents == EVENT_MOUSE_PRESS) {
			RayOrigin = mCamera.GetPosition();
			RayDirection = Ph::GenerateRayFromScreenCoordinates(Global::Projection, mCamera.GetViewMatrix(), vec2(e.mPayload.mClickX, e.mPayload.mClickY),
				vec2(mWindow->GetWidth(), mWindow->GetHeight()));
			if (e.mDetails == EVENT_DETAIL_RIGHT_BUTTON) {
				CameraMove = true;
				cursorRay.mOrigin = vec3(e.mPayload.mClickX, e.mPayload.mClickY, 0.0f);
				mUI->SetCursorPosition(cursorRay);
			}
			else if (e.mDetails == EVENT_DETAIL_LEFT_BUTTON) {
				sMouseSelect.bSelect = true;
				sMouseSelect.iA = ivec2(e.mPayload.mClickX, e.mPayload.mClickY);
				sMouseSelect.iA = sMouseSelect.iB;
			}
		}
		else if (e.mEvents == EVENT_MOUSE_RELEASE) {
			cursorRay.mOrigin = {};
			CameraMove = false;
			mUI->SetCursorPosition({});
			if (sMouseSelect.bSelect)
				sMouseSelect.mDeselected = true;
			sMouseSelect.bSelect = false;
		}
		}, nullptr);

	auto SCurve = [](float x, float A) {
		float invert = 1.0f;
		if (x < 0.0) {
			x *= -1.0f;
			invert = -1.0f;
		}
		float v = 1.0f / (1.0 + exp(-A * x + (A / 2.0)));
		return v * invert;
	};

	auto SCurve2 = [&](const glm::vec2& v) throw() -> glm::vec2 {
		float A = 15.0f;
		return glm::vec2(SCurve(v.x, A), SCurve(v.y, A));
	};

	nCallbackIDs[2] = mWindow->RegisterCallback(EVENT_MOUSE_MOVE, [&](const Event& e, void* pUserDefinied) throw() -> void {
		using namespace glm;
		if (cursorRay.mOrigin != vec3(0.0)) {
			cursorRay.mDirection = vec3(e.mPayload.mPositionX, e.mPayload.mPositionY, 0.0);
			mUI->SetCursorPosition(cursorRay);
			vec2 windowSize = vec2(Global::Window->GetWidth(), Global::Window->GetHeight());
			vec2 MaxMagnitude = vec2(windowSize - vec2(cursorRay.mOrigin));
			Magnitude = SCurve2(vec2(cursorRay.mDirection - cursorRay.mOrigin) / MaxMagnitude);
			Magnitude *= vec2(1.0f, -1.0f);
		}
		sMouseSelect.iB = ivec2(e.mPayload.mPositionX, e.mPayload.mPositionY);
		}, nullptr);
}

void CameraControl::LoadViewMatrix()
{
	if (mKeys[W_Key]) mCamera.MoveForward(Global::Time * 22.0);
	if (mKeys[S_Key]) mCamera.MoveForward(Global::Time * -22.0);
	if (mKeys[A_Key]) mCamera.MoveSideways(Global::Time * -22.0);
	if (mKeys[D_Key]) mCamera.MoveSideways(Global::Time * 22.0);
	if (mKeys[Q_Key]) mCamera.Yaw(Global::Time * -RotateRate, true);
	if (mKeys[E_Key]) mCamera.Yaw(Global::Time * RotateRate, true);
	if (mKeys[Z_Key]) mCamera.Pitch(Global::Time * -RotateRate, true);
	if (mKeys[X_Key]) mCamera.Pitch(Global::Time * RotateRate, true);
	if (mKeys[UP_Key]) mCamera.MoveAlongUpAxis(Global::Time * -22.0);
	if (mKeys[DOWN_Key]) mCamera.MoveAlongUpAxis(Global::Time * 22.0);
	if (CameraMove) {
		using namespace glm;
		vec3 position = mCamera.GetPosition();
		const float speed = 1000.0f;
		vec2 translation = vec2(speed) * float(Global::Time) * Magnitude;
		// camera may have been rotated so we need to adjust for the yaw rotation
		float yaw = radians(mCamera.GetYawDegrees());
		mat2 adjustment;
		adjustment[0][0] = cos(yaw);
		adjustment[1][0] = sin(yaw);
		adjustment[0][1] = -sin(yaw);
		adjustment[1][1] = cos(yaw);
		translation = adjustment * translation;
		mCamera.SetPosition(position + vec3(translation.x, 0.0, translation.y));
	}

	mCamera.UpdateViewMatrix();
	auto position = mCamera.GetPosition();
	UI::CameraPosition = position;
}
