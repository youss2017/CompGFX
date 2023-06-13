#include "CameraController.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <Utility/CppUtility.hpp>

using namespace egx::scene;

CameraController::CameraController() : vPosition(glm::vec3(0.0)), vLookDir(glm::vec3(0.0, 0.0, 1.0f)) {}

CameraController::CameraController(glm::vec3 vPosition, glm::vec3 vLookDir) : vPosition(vPosition), vLookDir(vLookDir) {}

void CameraController::Pitch(double dAngle, bool isDegree) {
	if (!isDegree) {
		dAngle = (dAngle * 180.0) / 3.142;
	}
	const double dMaxPitchAngle = 90.0f * 0.995f;
	dPitchAngle += dAngle;
	ComputeLookDir();
	//if (!ignoreClamps)
		//dPitchAngle = glm::clamp<double>(dPitchAngle, -dMaxPitchAngle, dMaxPitchAngle);
}

void CameraController::Yaw(double dAngle, bool isDegree) {
	if (!isDegree) {
		dAngle = (dAngle * 180.0) / glm::pi<double>();
	}
	dYawAngle += dAngle;
	ComputeLookDir();
	//if (!ignoreClamps) {
	//	if (dYawAngle > 360.0)
	//		dYawAngle = 0;
	//	if (dYawAngle < -360.0)
	//		dYawAngle = 0;
	//}
}

void CameraController::Move(glm::vec3 pos) {
	vPosition += pos;
}

void CameraController::SetPosition(glm::vec3 pos) {
	vPosition = { pos.x, pos.y, pos.z };
}

void CameraController::MoveForward(double dDistance) {
	glm::vec3 vForwardVector = ComputeLookDir();
	vForwardVector *= dDistance;
	vPosition += vForwardVector;
}

void CameraController::MoveSideways(double dDistance) {
	glm::vec3 forward = vLookDir;
	forward = glm::normalize(forward);
	
	//glm::vec3 a = forward * glm::dot({0.0, 1.0, 0.0}, forward);
	//glm::vec3 newUp = vec3{0.0, 1.0, 0.0} - a;
	glm::vec3 newUp = vUp;
	newUp = glm::normalize(newUp);

	glm::vec3 newRight = glm::cross(newUp, forward);
	newRight = glm::normalize(newRight);

	newRight *= dDistance;
	vPosition += newRight;
}

void CameraController::MoveAlongUpAxis(double dDistance) {
#if 0
	glm::vec3 _vLookDir = ComputeLookDir();
	glm::vec3 forward = (_vLookDir + vPosition) - vPosition;
	forward = glm::normalize(forward);
	glm::vec3 a = forward * glm::dot({0.0, 1.0, 0.0}, forward);
	glm::vec3 newUp = vec3{0.0, 1.0, 0.0} - a;
	newUp = glm::normalize(newUp);
#else
	vec3 newUp = vUp;
#endif
	newUp *= dDistance;
	vPosition += newUp;
}

void CameraController::MoveUp(double dDistance) {
	glm::vec3 upDistance = {0.0, 1.0, 0.0};
	upDistance *= dDistance;
	vPosition += upDistance;
}

glm::vec3 CameraController::GetPosition() noexcept {
	return vPosition;
}

glm::vec3 CameraController::ComputeLookDir() {
	glm::vec3 tmpLookDir = vLookDir;
	glm::mat4 rotX = glm::rotate(glm::mat4(1.0), glm::radians(float(-dPitchAngle)), glm::vec3(1.0, 0.0, 0.0));
	glm::mat4 rotY = glm::rotate(glm::mat4(1.0), glm::radians(float(-dYawAngle)), glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 rotation = rotX * rotY;
	vLookDir = rotation * glm::vec4(0.0, 0.0, 1.0, 1.0);
	vUp = rotate(mat4(1.0), radians(90.0f), { 1.0, 0.0, 0.0 }) * vec4(vLookDir, 1.0);
	return vLookDir;
}

double CameraController::GetYawDegrees() {
	return dYawAngle;
}

double CameraController::GetPitchDegrees() {
	return dPitchAngle;
}

const glm::mat4& CameraController::GetViewMatrix() {
	
	//glm::vec3 lookDir = ComputeLookDir();
	glm::vec3 target = vLookDir + vPosition;

	//vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	//vec3 up_from_lookdir = rotate(mat4(1.0), radians(90.0f), { 1.0, 0.0, 0.0 }) * vec4(lookDir, 1.0);

	//LOG(INFO, "up: <{:.2f},{:.2f},{:.2f}> up_lookdir: <{:.2f},{:.2f},{:.2f}>", up.x, up.y, up.z, up_from_lookdir.x, up_from_lookdir.y, up_from_lookdir.z);

	glm::mat4 lookAt = glm::lookAt(target, vPosition, vUp);

	mView = lookAt;
	return mView;
}

void CameraController::ResetCamera() {
	dPitchAngle = 0;
	dYawAngle = 0;
	vPosition = { 0, 0, 0 };
	vLookDir = { 0.0, 0.0, 1.0 };
	vUp = { 0.0, 1.0, 0.0 };
}