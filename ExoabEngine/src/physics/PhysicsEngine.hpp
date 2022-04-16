#pragma once
#include "PhysicsCore.hpp"

namespace Ph {

    struct Orientation {
        vec3 mPosition;
        vec3 mRotation;
        BoundingBox mBox;
    };

    class DynamicObject {

    public:
        DynamicObject(const Orientation orientation, float massKilograms, vec3 initalSpeedMetersPerSecond, vec3 initalVelocityMetersPerSecond, vec3 initalAcclerationMetersPerSecond);
        ~DynamicObject();

        DynamicObject(const DynamicObject& copy) = 0;
        DynamicObject(const DynamicObject&& move) = 0;

        // Call this before PhysicsEngine::Update()
        void ApplyForce(vec3 forceInNewtons);

        Orientation GetUpdatedStatus();

    private:
        friend class PhysicsEngine;
        Orientation mOrientation;
        float mMass;
        vec3 mSpeed;
        vec3 mVelocity;
        vec3 mAcceleration;
        vec3 mForce;
    };

    class PhysicsEngine {

    public:
        PhysicsEngine(vec3 Cube1x1MeterPixelScale, vec3 gravity, float timeStepInSeconds);
        ~PhysicsEngine();

        PhysicsEngine(const PhysicsEngine& copy) = 0;
        PhysicsEngine(const PhysicsEngine&& move) = 0;

        void AddDynamicObject(DynamicObject* obj);

        void Update();

    private:


    };

}
