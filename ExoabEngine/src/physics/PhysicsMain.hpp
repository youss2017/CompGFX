#pragma once
#include <PxPhysicsAPI.h>

class PhysicsMain
{
public:
	PhysicsMain();
	~PhysicsMain();
private:
	// Requirement for all physx objects
	physx::PxFoundation* m_foundation;
	// Allows us to connect (via socket) to NVIDIA visual debugger.
	physx::PxPvd* m_visual_debugger;
	physx::PxPhysics* m_physics;
};

