#include "CameraController.hpp"
#include <glm/gtc/matrix_transform.hpp>
using namespace shared::components;
using namespace shared::scene;

CameraController::CameraController(const std::shared_ptr<shared::scene::SceneData>& sceneData, const egx::ref<egx::PlatformWindow>& window)
	: ISceneLogicComponent(sceneData), _Window(window)
{
	SetSceneData("CameraController::View", glm::mat4(1.0));
	_Width = window->GetWidth(), _Height = window->GetHeight();
	SetFov(_Fov);
}

void CameraController::SetFov(double angleDegrees) {
	_Fov = angleDegrees;
	SetSceneData("CameraController::Proj", glm::perspectiveFovLH<float>(90.0f, float(_Width), float(_Height), 0.1f, 1000.0f));
}

void CameraController::SetResolution(int32_t width, int32_t height)
{
	SetFov(_Fov);
}

void CameraController::Update(double dTime)
{
	auto& window = _Window;
	auto& c = Camera;
	_SceneData->SetSceneDataPointer("CameraController::Camera", &c);

	double dMov = dTime * 10.0;
	double dAngle = dTime * 35.0;
	if (window->GetKeyState(GLFW_KEY_UP))
	{
		c.MoveAlongUpAxis(-dMov);
	}
	if ((window->GetKeyState(GLFW_KEY_DOWN)))
	{
		c.MoveAlongUpAxis(dMov);
	}
	if (window->GetKeyState(GLFW_KEY_LEFT) || window->GetKeyState('A'))
	{
		c.MoveSideways(-dMov);
	}
	if (window->GetKeyState(GLFW_KEY_RIGHT) || window->GetKeyState('D'))
	{
		c.MoveSideways(dMov);
	}
	if (window->GetKeyState('W'))
	{
		c.MoveForward(dMov);
	}
	if (window->GetKeyState('S'))
	{
		c.MoveForward(-dMov);
	}
	if (window->GetKeyState('Q'))
	{
		c.Yaw(-dAngle, true);
	}
	if (window->GetKeyState('E'))
	{
		c.Yaw(dAngle, true);
	}
	if (window->GetKeyState('Z'))
	{
		c.Pitch(-dAngle, true);
	}
	if (window->GetKeyState('X'))
	{
		c.Pitch(dAngle, true);
	}

	c.UpdateViewMatrix();
	SetSceneData("CameraController::View", c.GetViewMatrix());
}
