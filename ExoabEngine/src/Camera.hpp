#pragma once
#include "core/MaxEnergyMath.h"
#include <glm/glm.hpp>

// TODO: Replace this with glm

class Camera {

private:
	Vec3 vLookDir;
	Vec3 vPosition;
	const Vec3 vUp = { 0, 1, 0 };

	double dPitchAngle = 0.0;
	double dYawAngle = 0.0;

public:
	Camera(Vec3 vPosition, Vec3 vLookDir = { 0, 0, 1 });

	void Pitch(double dAngle, bool isDegree);
	void Yaw(double dAngle, bool isDegree);

	void Move(Vec3 pos);
	void SetPosition(Vec3 pos);

	void MoveForward(double dDistance);

	void MoveSideways(double dDistance);

	void MoveAlongUpAxis(double dDistance);
	void MoveUp(double dDistance);
	Vec3 GetPosition() noexcept;

	Vec3 GetLookDir();

	double GetYawDegrees();

	double GetPitchDegrees();

	glm::mat4 GetViewMatrix();

	void ResetCamera();

};