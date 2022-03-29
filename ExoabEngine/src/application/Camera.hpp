#pragma once
#include <glm/glm.hpp>

class Camera {

private:
	glm::vec3 vLookDir;
	glm::vec3 vPosition;
	const glm::vec3 vUp = { 0, 1, 0 };

	double dPitchAngle = 0.0;
	double dYawAngle = 0.0;

public:
	Camera(glm::vec3 vPosition, glm::vec3 vLookDir = { 0, 0, 1 });

	void Pitch(double dAngle, bool isDegree);
	void Yaw(double dAngle, bool isDegree);

	void Move(glm::vec3 pos);
	void SetPosition(glm::vec3 pos);

	void MoveForward(double dDistance);

	void MoveSideways(double dDistance);

	void MoveAlongUpAxis(double dDistance);
	void MoveUp(double dDistance);
	glm::vec3 GetPosition() noexcept;

	glm::vec3 GetLookDir();

	double GetYawDegrees();

	double GetPitchDegrees();

	glm::mat4 GetViewMatrix();

	void ResetCamera();

};