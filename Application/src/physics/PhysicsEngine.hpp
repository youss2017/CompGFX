#pragma once
#include "PhysicsCore.hpp"
#include <vector>

namespace Ph {

    // Plane Equation --> Ax + By + Cz + D = 0
    struct Plane {
        vec3 mNormal;
        float mD;
    };

    struct Orientation {
        vec3 mPosition;
        vec3 mRotation;
        BoundingBox mBox;

        static Orientation Init(vec3 position, vec3 rotation, BoundingBox box) {
            return { position, rotation, box };
        }
    };

    class DynamicObject {

    public:
        DynamicObject(const Orientation orientation, float massKilograms, vec3 initalVelocityMetersPerSecond);
        ~DynamicObject();

        DynamicObject(const DynamicObject& copy) = delete;
        DynamicObject(const DynamicObject&& move) = delete;

        // Call this before PhysicsEngine::Update()
        void ApplyForce(vec3 forceInNewtons);
        void ApplyAcceleration(vec3 accelerationInMPerSeconds);

        Orientation GetUpdatedStatus();

    private:
        friend class PhysicsEngine;
        Orientation mOrientation;
        float mMass;
        vec3 mVelocity;
        vec3 mForce;
    };

    class PhysicsEngine {

    public:
        PhysicsEngine(float Cube1x1MeterPixelScale, vec3 gravity);
        ~PhysicsEngine();

        PhysicsEngine(const PhysicsEngine& copy) = delete;
        PhysicsEngine(const PhysicsEngine&& move) = delete;

        void AddDynamicObject(DynamicObject* obj);
        void AddPlane(const Plane& plane);

        void Update();
                
    private:
        float mCubeScale;
        vec3 mGravity;
        std::vector<Plane> mPlanes;
        std::vector<DynamicObject*> mDynamicObjects;
    };

}
