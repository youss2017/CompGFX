#include "Camera.hpp"

Camera::Camera(Vec3 vPosition, Vec3 vLookDir) : vPosition(vPosition), vLookDir(vLookDir) {}

void Camera::Pitch(double dAngle, bool isDegree) {
	if (!isDegree) {
		dAngle = (dAngle * 180.0) / 3.142;
	}
	const double dMaxPitchAngle = 90.0f * 0.995f;
	dPitchAngle += dAngle;
	dPitchAngle = clamp<double>(dPitchAngle, -dMaxPitchAngle, dMaxPitchAngle);
}

void Camera::Yaw(double dAngle, bool isDegree) {
	if (!isDegree) {
		dAngle = (dAngle * 180.0) / 3.142;
	}
	dYawAngle += dAngle;
	if (dYawAngle > 360.0)
		dYawAngle = 0;
	if (dYawAngle < -360.0)
		dYawAngle = 0;
}

void Camera::Move(Vec3 pos) {
	vPosition.add(pos);
}

void Camera::SetPosition(Vec3 pos) {
	vPosition = { pos.x, pos.y, pos.z };
}

void Camera::MoveForward(double dDistance) {
	Vec3 vForwardVector = GetLookDir();
	vForwardVector.mul(dDistance);
	vPosition.add(vForwardVector);
}

void Camera::MoveSideways(double dDistance) {
	Vec3 _vLookDir = GetLookDir();
	Vec3 forward = Vec3::Sub(_vLookDir.add(vPosition), vPosition);
	forward.normalize();
	Vec3 vUp = { 0, 1, 0 };
	
	Vec3 a = Vec3::Mul(forward, vUp.dot(forward));
	Vec3 newUp = Vec3::Sub(vUp, a);
	newUp.normalize();

	Vec3 newRight = Vec3::crossproduct(newUp, forward);
	newRight.normalize();

	newRight.mul(dDistance);
	vPosition.add(newRight);
}

void Camera::MoveAlongUpAxis(double dDistance) {
	Vec3 _vLookDir = GetLookDir();
	Vec3 forward = Vec3::Sub(_vLookDir.add(vPosition), vPosition);
	forward.normalize();
	Vec3 vUp = { 0, 1, 0 };
	Vec3 a = Vec3::Mul(forward, vUp.dot(forward));
	Vec3 newUp = Vec3::Sub(vUp, a);
	newUp.normalize();

	newUp.mul(dDistance);

	vPosition.add(newUp);
}

void Camera::MoveUp(double dDistance) {
	Vec3 vUp = { 0, 1, 0 };
	vUp.mul(dDistance);
	vPosition.add(vUp);
}

Vec3 Camera::GetPosition() noexcept {
	return vPosition;
}

Vec3 Camera::GetLookDir() {
	Vec3 tmpLookDir = vLookDir;
	Mat4 rotX = Mat4::MakeRotationX(dPitchAngle, true);
	Mat4 rotY = Mat4::MakeRotationY(dYawAngle, true);
	Mat4 rotation = rotY * rotX;
	Mat4::MulVec4ByMat4(tmpLookDir, tmpLookDir, rotation);
	return tmpLookDir;
}

double Camera::GetYawDegrees() {
	return dYawAngle;
}

double Camera::GetPitchDegrees() {
	return dPitchAngle;
}

//#include <DirectXMath.h>

glm::mat4	 Camera::GetViewMatrix() {
	Vec3 tmpLookDir = GetLookDir();
	Vec3 vTarget = Vec3::Add(vPosition, tmpLookDir);
	/*DirectX::XMVECTOR eyeposition;
	DirectX::XMVECTOR target;
	DirectX::XMVECTOR updirection;

	DirectX::XMMATRIX lookat = 
		DirectX::XMMatrixTranspose(
		DirectX::XMMatrixLookAtLH(
		DirectX::XMVectorSet(vPosition.x, vPosition.y, vPosition.z, 1.0),
		DirectX::XMVectorSet(vTarget.x, vTarget.y, vTarget.z, 1.0),
		DirectX::XMVectorSet(vUp.x, vUp.y, vUp.z, 1.0))
		);

	Mat4 test = *((Mat4*)&lookat);*/
	Mat4 lookAt =  Mat4::MakeLookAtMatrix(vPosition, vUp, vTarget);
	lookAt = lookAt.Transpose(lookAt);
	glm::mat4 la = *(glm::mat4*)&lookAt;
	return la;
}

void Camera::ResetCamera() {
	dPitchAngle = 0;
	dYawAngle = 0;
	vPosition = { 0, 0, 0 };
}