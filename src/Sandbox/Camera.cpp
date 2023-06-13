#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera() : vPosition(glm::vec3(0.0)), vLookDir(glm::vec3(0.0, 0.0, 1.0f)) {}

Camera::Camera(glm::vec3 vPosition, glm::vec3 vLookDir) : vPosition(vPosition), vLookDir(vLookDir) {}

void Camera::Pitch(double dAngle, bool isDegree, bool ignoreClamps) {
	if (!isDegree) {
		dAngle = (dAngle * 180.0) / 3.142;
	}
	const double dMaxPitchAngle = 90.0f * 0.995f;
	dPitchAngle += dAngle;
	if (!ignoreClamps)
		dPitchAngle = glm::clamp<double>(dPitchAngle, -dMaxPitchAngle, dMaxPitchAngle);
}

void Camera::Yaw(double dAngle, bool isDegree, bool ignoreClamps) {
	if (!isDegree) {
		dAngle = (dAngle * 180.0) / glm::pi<double>();
	}
	dYawAngle += dAngle;
	if (!ignoreClamps) {
		if (dYawAngle > 360.0)
			dYawAngle = 0;
		if (dYawAngle < -360.0)
			dYawAngle = 0;
	}
}

void Camera::Move(glm::vec3 pos) {
	vPosition += pos;
}

void Camera::SetPosition(glm::vec3 pos) {
	vPosition = { pos.x, pos.y, pos.z };
}

void Camera::MoveForward(double dDistance) {
	glm::vec3 vForwardVector = ComputeLookDir();
	vForwardVector *= dDistance;
	vPosition += vForwardVector;
}

void Camera::MoveSideways(double dDistance) {
	glm::vec3 _vLookDir = ComputeLookDir();
	glm::vec3 forward = (_vLookDir + vPosition) - vPosition;
	forward = glm::normalize(forward);
	
	glm::vec3 a = forward * glm::dot(vUp, forward);
	glm::vec3 newUp = vUp - a;
	newUp = glm::normalize(newUp);

	glm::vec3 newRight = glm::cross(newUp, forward);
	newRight = glm::normalize(newRight);

	newRight *= dDistance;
	vPosition += newRight;
}

void Camera::MoveAlongUpAxis(double dDistance) {
	glm::vec3 _vLookDir = ComputeLookDir();
	glm::vec3 forward = (_vLookDir + vPosition) - vPosition;
	forward = glm::normalize(forward);
	glm::vec3 a = forward * glm::dot(vUp, forward);
	glm::vec3 newUp = vUp - a;
	newUp = glm::normalize(newUp);

	newUp *= dDistance;

	vPosition += newUp;
}

void Camera::MoveUp(double dDistance) {
	glm::vec3 upDistance = vUp;
	upDistance *= dDistance;
	vPosition += upDistance;
}

glm::vec3 Camera::GetPosition() noexcept {
	return vPosition;
}

glm::vec3 Camera::ComputeLookDir() {
	glm::vec3 tmpLookDir = vLookDir;
	glm::mat4 rotX = glm::rotate(glm::mat4(1.0), glm::radians(float(-dPitchAngle)), glm::vec3(1.0, 0.0, 0.0));
	glm::mat4 rotY = glm::rotate(glm::mat4(1.0), glm::radians(float(-dYawAngle)), glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 rotation = rotX * rotY;
	return glm::vec4(tmpLookDir, 1.0) * rotation;
}

double Camera::GetYawDegrees() {
	return dYawAngle;
}

double Camera::GetPitchDegrees() {
	return dPitchAngle;
}

const glm::mat4& Camera::UpdateViewMatrix(bool applyPositionTranslation) {
	glm::vec3 tmpLookDir = ComputeLookDir();
	glm::vec3 vTarget = tmpLookDir + vPosition;
	glm::mat4 lookAt = glm::lookAt(vTarget, vPosition, vUp);
	mView = lookAt;
	return mView;
}

const glm::mat4& Camera::GetViewMatrix() {
	
	return mView;
}

void Camera::ResetCamera() {
	dPitchAngle = 0;
	dYawAngle = 0;
	vPosition = { 0, 0, 0 };
}