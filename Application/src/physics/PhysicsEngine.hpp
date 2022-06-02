#pragma once
#include "PhysicsCore.hpp"
#include "ecs/Entity.hpp"
#include "PxPhysics.h"
#include "PxFoundation.h"
#include "cooking/PxCooking.h"
#include "extensions/PxDefaultCpuDispatcher.h"
#include "../mesh/Terrain.hpp"
#include <vector>

namespace Ph {

    // Plane Equation --> Ax + By + Cz + D = 0
    struct Plane {
        vec3 mNormal;
        float mD;
    };

    bool BBPlaneIntersect(const Plane& plane, const BoundingBox& box);
    
    class PhysicsEntity : public ecs::Entity {
    public:
        void Update();
        
    public:
        const bool bDynamic;
        physx::PxSphereGeometry mBoundingSphere;

    protected:
        friend class PhysicsEngine;
        PhysicsEntity(bool bDynamic, physx::PxShape* shape, physx::PxRigidActor* actor) : bDynamic(bDynamic), mSphereShape(shape), mActor(actor) {}
        ~PhysicsEntity();

        physx::PxShape* mSphereShape;
        physx::PxRigidActor* mActor;
    };

    class PhysicsEngine {

    public:
        PhysicsEngine(float Cube1x1MeterPixelScale, vec3 gravity);
        ~PhysicsEngine();

        PhysicsEngine(const PhysicsEngine& copy) = delete;
        PhysicsEngine(const PhysicsEngine&& move) = delete;

        // This is managed by PhysicsEngine, will be deleted inside deconstructor
        // a material describes the physical properties of the actor, returns -1 on failure
        int CreateMaterial(float staticFriction, float dynamicFriction, float restitution);
        PhysicsEntity* CreateRigidEntity(ecs::IEntityGeometry eg, const ShaderTypes::InstanceData& data, int nMaterialID, bool bDynamic);
        void DestroyRigidEntity(PhysicsEntity* e);
        void AddPlane(vec4 plane, int nMaterialID);

        void SetTerrain(Terrain* terrain);

        void Start();
        void End();

    private:
        physx::PxTolerancesScale mScale;
        physx::PxPhysics* mPhysics;
        physx::PxFoundation* mFoundation;
        physx::PxPvd* mPVD;
        physx::PxCooking* mCooking;
        physx::PxDefaultCpuDispatcher* mCpuDispatcher;
        // Everything goes in this scene.
        physx::PxScene* mMasterScene;
        std::vector<physx::PxMaterial*> vMaterials;
        std::vector<physx::PxRigidStatic*> vPlanes;
    };

}
