#include "CameraController.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <Utility/CppUtility.hpp>

using namespace egx::scene;

CameraController::CameraController() : vPosition(glm::vec3(0.0)), vLookDir(glm::vec3(0.0, 0.0, 1.0f)) {}

CameraController::CameraController(glm::vec3 vPosition, glm::vec3 vLookDir) : vPosition(vPosition), vLookDir(vLookDir) {}

void CameraController::Pitch(double dAngle, bool isDegree)
{
	if (!isDegree)
	{
		dAngle = (dAngle * 180.0) / 3.142;
	}
	const double dMaxPitchAngle = 90.0f * 0.995f;
	dPitchAngle += dAngle;
	ComputeLookDir();
	// if (!ignoreClamps)
	// dPitchAngle = glm::clamp<double>(dPitchAngle, -dMaxPitchAngle, dMaxPitchAngle);
}

void CameraController::Yaw(double dAngle, bool isDegree)
{
	if (!isDegree)
	{
		dAngle = (dAngle * 180.0) / glm::pi<double>();
	}
	dYawAngle += dAngle;
	ComputeLookDir();
	// if (!ignoreClamps) {
	//	if (dYawAngle > 360.0)
	//		dYawAngle = 0;
	//	if (dYawAngle < -360.0)
	//		dYawAngle = 0;
	// }
}

void CameraController::Move(glm::vec3 pos)
{
	vPosition += pos;
}

void CameraController::SetPosition(glm::vec3 pos)
{
	vPosition = {pos.x, pos.y, pos.z};
}

void CameraController::MoveForward(double dDistance)
{
	glm::vec3 vForwardVector = ComputeLookDir();
	vForwardVector *= dDistance;
	vPosition += vForwardVector;
}

void CameraController::MoveSideways(double dDistance)
{
	glm::vec3 forward = vLookDir;
	forward = glm::normalize(forward);

	// glm::vec3 a = forward * glm::dot({0.0, 1.0, 0.0}, forward);
	// glm::vec3 newUp = vec3{0.0, 1.0, 0.0} - a;
	glm::vec3 newUp = vUp;
	newUp = glm::normalize(newUp);

	glm::vec3 newRight = glm::cross(newUp, forward);
	newRight = glm::normalize(newRight);

	newRight *= dDistance;
	vPosition += newRight;
}

void CameraController::MoveAlongUpAxis(double dDistance)
{
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

void CameraController::MoveUp(double dDistance)
{
	glm::vec3 upDistance = {0.0, 1.0, 0.0};
	upDistance *= dDistance;
	vPosition += upDistance;
}

glm::vec3 CameraController::GetPosition() noexcept
{
	return vPosition;
}

glm::vec3 CameraController::ComputeLookDir()
{
	glm::vec3 tmpLookDir = vLookDir;
	glm::mat4 rotX = glm::rotate(glm::mat4(1.0), glm::radians(float(-dPitchAngle)), glm::vec3(1.0, 0.0, 0.0));
	glm::mat4 rotY = glm::rotate(glm::mat4(1.0), glm::radians(float(-dYawAngle)), glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 rotation = rotX * rotY;
	vLookDir = rotation * glm::vec4(0.0, 0.0, 1.0, 1.0);
	vUp = rotate(mat4(1.0), radians(90.0f), {1.0, 0.0, 0.0}) * vec4(vLookDir, 1.0);
	return vLookDir;
}

double CameraController::GetYawDegrees()
{
	return dYawAngle;
}

double CameraController::GetPitchDegrees()
{
	return dPitchAngle;
}

const glm::mat4 &CameraController::GetViewMatrix()
{

	// glm::vec3 lookDir = ComputeLookDir();
	glm::vec3 target = vLookDir + vPosition;

	// vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	// vec3 up_from_lookdir = rotate(mat4(1.0), radians(90.0f), { 1.0, 0.0, 0.0 }) * vec4(lookDir, 1.0);

	// LOG(INFO, "up: <{:.2f},{:.2f},{:.2f}> up_lookdir: <{:.2f},{:.2f},{:.2f}>", up.x, up.y, up.z, up_from_lookdir.x, up_from_lookdir.y, up_from_lookdir.z);

	glm::mat4 lookAt = glm::lookAt(target, vPosition, vUp);

	mView = lookAt;
	return mView;
}

void CameraController::ResetCamera()
{
	dPitchAngle = 0;
	dYawAngle = 0;
	vPosition = {0, 0, 0};
	vLookDir = {0.0, 0.0, 1.0};
	vUp = {0.0, 1.0, 0.0};
}

CameraController &egx::scene::CameraController::ConfigureControls(
	uint16_t forward_key, uint16_t backward_key, 
	uint16_t left_key, uint16_t right_key, 
	uint16_t up_key, uint16_t down_key,
	uint16_t yaw_left_key, uint16_t yaw_right_key, 
	uint16_t pitch_up_key, uint16_t pitch_down_key)
{
	m_forward_key = forward_key, m_backward_key = backward_key,
	m_left_key = left_key, m_right_key = right_key,
	m_up_key = up_key, m_down_key = down_key,
	m_left_yaw_key = yaw_left_key, m_right_yaw_key = yaw_right_key,
	m_pitch_up_key = pitch_up_key, m_pitch_down_key = pitch_down_key;
	return *this;
}

CameraController &egx::scene::CameraController::ConfigureMotionalSpeeds(
	double forward_speed, double backward_speed, 
	double left_speed, double right_speed,
	double up_speed, double down_speed,
	double yaw_angle_per_second, double pitch_angle_per_second)
{
    m_forward_speed = forward_speed;
    m_backward_speed = backward_speed;
    m_left_speed = left_speed;
    m_right_speed = right_speed;
    m_up_speed = up_speed;
    m_down_speed = down_speed;
    m_yaw_angle_per_second = yaw_angle_per_second;
    m_pitch_angle_per_second = pitch_angle_per_second;
	return *this;
}

std::function<void(const Event &, void *)> egx::scene::CameraController::GetKeyboardCallback()
{
	return [this](const Event &e, void *pUserData)
	{
		if (e.mDetails & EVENT_FLAGS_KEY_PRESS || e.mDetails & EVENT_FLAGS_KEY_RELEASE)
		{
			this->m_pressed_keys[e.mPayload.NonASCIKey] = e.mEvents & EVENT_FLAGS_KEY_PRESS;
		}
	};
}

void egx::scene::CameraController::ProcessMotion(double deltaTime)
{
	if (
		m_forward_key >= last_key ||
		m_backward_key >= last_key ||
		m_left_key >= last_key ||
		m_right_key >= last_key ||
		m_left_yaw_key >= last_key ||
		m_right_yaw_key >= last_key ||
		m_pitch_up_key >= last_key ||
		m_pitch_down_key >= last_key)
	{
		LOG(WARNING, "Cannot process user input because one of the configured keys in camera controller is not ascii and not VK-code key. (key value must be less than {})", last_key);
		return;
	}

	if (m_pressed_keys[m_forward_key])
	{
		MoveForward(-m_forward_speed * deltaTime);
	}
	if (m_pressed_keys[m_backward_key])
	{
		MoveForward(m_backward_speed * deltaTime);
	}
	if (m_pressed_keys[m_left_key])
	{
		MoveSideways(-m_left_speed* deltaTime);
	}
	if (m_pressed_keys[m_right_key])
	{
		MoveSideways(m_right_speed * deltaTime);
	}
	if (m_pressed_keys[m_up_key])
	{
		MoveAlongUpAxis(m_up_speed * deltaTime);
	}
	if (m_pressed_keys[m_down_key])
	{
		MoveAlongUpAxis(-m_down_speed * deltaTime);
	}
	if (m_pressed_keys[m_left_yaw_key])
	{
		Yaw(m_yaw_angle_per_second * deltaTime, true);
	}
	if (m_pressed_keys[m_right_yaw_key])
	{
		Yaw(-m_yaw_angle_per_second * deltaTime, true);
	}
	if (m_pressed_keys[m_pitch_up_key])
	{
		Pitch(-m_pitch_angle_per_second * deltaTime, true);
	}
	if (m_pressed_keys[m_pitch_down_key])
	{
		Pitch(m_pitch_angle_per_second * deltaTime, true);
	}
}
