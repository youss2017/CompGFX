#pragma once
#include <Shared/scene/Scene2.hpp>
#include <Shared/Camera.hpp>
#include <egx/window/PlatformWindow.hpp>
#include <egx/window/Event.hpp>

namespace shared::components {

	class CameraController : public shared::scene::ISceneLogicComponent {
	public:
		CameraController(const std::shared_ptr<shared::scene::SceneData>& sceneData, const egx::ref<egx::PlatformWindow>& window);

		// ViewMatrix
		// CameraController::View
		// ProjMatrix
		// CameraController::Proj

		void SetFov(double angleDegrees);
		void SetResolution(int32_t width, int32_t height);

		// SceneDataPointer = CameraController::Camera
		shared::Camera Camera;
	protected:
		virtual void Update(double dTime) override;

	private:
		egx::ref<egx::PlatformWindow> _Window;
		double _Fov = 90.0;
		int32_t _Width = 0;
		int32_t _Height = 0;
	};

}

