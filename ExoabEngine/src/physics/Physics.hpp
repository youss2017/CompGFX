#pragma once
#include <PxPhysicsAPI.h>
#include <geometry/PxConvexMesh.h>
#include <vehicle/PxVehicleSDK.h>
#include <foundation/PxAllocatorCallback.h>
#include <foundation/PxErrorCallback.h>
#include <glm/vec3.hpp>
#include "../mesh/geometry.hpp"

extern physx::PxFoundation* gFoundation;
extern physx::PxPhysics* gPhysics;
extern physx::PxPvd* gPvd;
extern physx::PxCooking* gCooking;
extern physx::PxDefaultCpuDispatcher* gDispatcher;
extern physx::PxScene* gScene;

bool Physx_Initalize();
void Physx_Destroy();
void Physx_Simulate();

physx::PxMaterial* Physx_CreateMaterial(float staticFriction, float dynamicFriction, float restitution);
physx::PxShape* Physx_CreateShape(physx::PxGeometry geometry, physx::PxMaterial* material);
physx::PxTriangleMesh* Physx_CreateTriangleMesh(Mesh::Geometry& geometry);
physx::PxRigidDynamic* Physx_CreateDynamicActor(physx::PxMaterial* material, physx::PxTriangleMesh* mesh, glm::vec3 scale, physx::PxTransform pose);
