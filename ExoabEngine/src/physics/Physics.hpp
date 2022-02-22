#pragma once
#include <PxPhysicsAPI.h>
#include <geometry/PxConvexMesh.h>
#include <vehicle/PxVehicleSDK.h>
#include <foundation/PxAllocatorCallback.h>
#include <foundation/PxErrorCallback.h>
#include "../mesh/geometry.hpp"

extern physx::PxFoundation* gFoundation;
extern physx::PxPhysics* gPhysics;
extern physx::PxPvd* gPvd;
extern physx::PxCooking* gCooking;
extern physx::PxDefaultCpuDispatcher* gDispatcher;

bool Physx_Initalize();
void Physx_Destroy();
