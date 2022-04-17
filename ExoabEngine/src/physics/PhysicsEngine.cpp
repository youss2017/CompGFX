#include "PhysicsEngine.hpp"
#include "Globals.hpp"

// F = ma
// F : N --> (kg * m) / s^2
// m : kg
// a : m / s^2


namespace Ph {

    DynamicObject::DynamicObject(const Orientation orientation, float massKilograms, vec3 initalVelocityMetersPerSecond, vec3 initalAcclerationMetersPerSecond)
    {
        assert(massKilograms > 0);
        mOrientation = orientation;
        mMass = massKilograms;
        mVelocity = initalVelocityMetersPerSecond;
        mAcceleration = initalAcclerationMetersPerSecond;
        mForce = vec3(0.0);
    }

    DynamicObject::~DynamicObject()
    {}
    
    // Call this before PhysicsEngine::Update()
    void DynamicObject::ApplyForce(vec3 forceInNewtons)
    {
        mForce = forceInNewtons;
    }

    Orientation DynamicObject::GetUpdatedStatus() { 
        return mOrientation;
    }

    PhysicsEngine::PhysicsEngine(vec3 Cube1x1MeterPixelScale, vec3 gravity, float timeStepInSeconds)
    {
        mCubeScale = Cube1x1MeterPixelScale;
        mGravity = gravity;
        mTimeStep = timeStepInSeconds;
    }

    PhysicsEngine::~PhysicsEngine()
    {}

    void PhysicsEngine::AddPlane(const Plane& plane)
    {
        mPlanes.push_back(plane);
    }

    void PhysicsEngine::AddDynamicObject(DynamicObject* obj)
    {
        mDynamicObjects.push_back(obj);
    }

    void PhysicsEngine::Update()
    {
        float dTime = Global::Time;
        
        auto BBPlaneIntersect = [](const Plane& plane, const BoundingBox& box) throw() -> bool {
            vec3 points[8];
            points[0] = box.mBoxMin;
            points[1] = vec3(box.mBoxMax.x, box.mBoxMin.y, box.mBoxMin.z);
            points[2] = vec3(box.mBoxMax.x, box.mBoxMin.y, box.mBoxMax.z);
            points[3] = vec3(box.mBoxMin.x, box.mBoxMin.y, box.mBoxMax.z);
            points[4] = vec3(box.mBoxMax.x, box.mBoxMax.y, box.mBoxMax.z);
            points[5] = vec3(box.mBoxMin.x, box.mBoxMax.y, box.mBoxMax.z);
            points[6] = vec3(box.mBoxMin.x, box.mBoxMin.y, box.mBoxMax.z);
            points[7] = vec3(box.mBoxMax.x, box.mBoxMin.y, box.mBoxMax.z);
            for(int i = 0; i < 8; i++)
                if (dot(plane.mNormal, points[i]) + plane.mD < 0)
                    return true;
            return false;
        };
        
        while(dTime > 0) {
            for(DynamicObject* obj : mDynamicObjects) {
                vec3& force = obj->mForce;
                vec3& acceleration = obj->mAcceleration;
                if(length(force) > 0) {
                    // apply force
                    acceleration += (force / obj->mMass) * mTimeStep;
                    force -= force * mTimeStep;
                }                
                // apply gravity force
                acceleration += mGravity * mTimeStep;
                acceleration -= acceleration * 0.015f * mTimeStep; // air drag?
                // apply acceleration
                vec3& velocity = obj->mVelocity;
                velocity += acceleration * mTimeStep;
                vec3& position = obj->mOrientation.mPosition;
                // convert position to meters scale
                position /= mCubeScale;
                vec3 movement = velocity * mTimeStep;
                position += movement;
                for(auto& plane : mPlanes) {
                    BoundingBox box = obj->mOrientation.mBox;
                    box.mBoxMin += position;
                    box.mBoxMax += position;
                    if (BBPlaneIntersect(plane, box)) {
                        position -= movement;
                    }
                }
                // convert position from meters scale to world space
                position *= mCubeScale;
            }
            dTime -= mTimeStep;
        }
    }

}