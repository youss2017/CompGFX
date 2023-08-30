#pragma once
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <window/Event.hpp>
#include <functional>

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

        // only ascii keys
        CameraController &ConfigureControls(uint16_t forward_key = 'W', uint16_t backward_key = 'S',
                                            uint16_t left_key = 'A', uint16_t right_key = 'D',
                                            uint16_t up_key = 265, uint16_t down_key = 264,
                                            uint16_t yaw_left_key = 'Q', uint16_t yaw_right_key = 'E',
                                            uint16_t pitch_up_key = 'X', uint16_t pitch_down_key = 'Z');

        CameraController &ConfigureMotionalSpeeds(double forward_speed, double backward_speed,
                                                  double left_speed, double right_speed,
                                                  double up_speed, double down_speed,
                                                  double yaw_angle_per_second,
                                                  double pitch_angle_per_second);

        std::function<void(const Event &, void *)> GetKeyboardCallback();
        void ProcessMotion(double deltaTime);

    private:
        static const size_t last_key = 348 /* GLFW_KEY_LAST */;

        glm::vec3 vLookDir;
        glm::vec3 vPosition;
        glm::vec3 vUp;

        double dPitchAngle = 0.0;
        double dYawAngle = 0.0;
        glm::mat4 mView = glm::mat4(1.0);

        uint16_t m_forward_key = 'W';
        uint16_t m_backward_key = 'S';
        uint16_t m_left_key = 'A';
        uint16_t m_right_key = 'D';
        uint16_t m_up_key = 265;
        uint16_t m_down_key = 264;
        uint16_t m_left_yaw_key = 'Q';
        uint16_t m_right_yaw_key = 'E';
        uint16_t m_pitch_up_key = 'X';
        uint16_t m_pitch_down_key = 'Z';
        bool m_pressed_keys[last_key] = {};
        double m_forward_speed = 2.0;
        double m_backward_speed = 2.0;
        double m_left_speed = 2.0;
        double m_up_speed = 2.0;
        double m_down_speed = 2.0;
        double m_right_speed = 2.0;
        double m_yaw_angle_per_second = 45.0;
        double m_pitch_angle_per_second = 45.0;
    };
}
