#pragma once
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace egx::scene
{
    using namespace glm;

    class CameraController
    {
    public:
        CameraController();
        CameraController(glm::vec3 vPosition, glm::vec3 vLookDir = {0, 0, 1});

        void Pitch(double dAngle, bool degree);
        void Yaw(double dAngle, bool degree);

        void Move(glm::vec3 pos);
        void SetPosition(glm::vec3 pos);

        void MoveForward(double dDistance);

        void MoveSideways(double dDistance);

        void MoveAlongUpAxis(double dDistance);
        void MoveUp(double dDistance);
        glm::vec3 GetPosition() noexcept;

        glm::vec3 ComputeLookDir();

        double GetYawDegrees();

        double GetPitchDegrees();

        const glm::mat4 &GetViewMatrix();

        void ResetCamera();
        
    private:
        glm::vec3 vLookDir;
        glm::vec3 vPosition;
        glm::vec3 vUp;

        double dPitchAngle = 0.0;
        double dYawAngle = 0.0;
        glm::mat4 mView = glm::mat4(1.0);
    };

}
