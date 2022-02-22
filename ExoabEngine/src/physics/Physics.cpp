#include "Physics.hpp"

#include "../utils/Logger.hpp"
#include <string>
#include <sstream>
#include <cassert>
// To specifiy directory of dlls
#include <Windows.h>
#include <iostream>
#include <meshoptimizer/src/meshoptimizer.h>

using namespace physx;

class PhysicsHostMemoryAllocator : public PxAllocatorCallback
{
public:
    ~PhysicsHostMemoryAllocator() {}
    void* allocate(size_t size, const char* typeName, const char* filename,
        int line)
    {
        return _aligned_malloc(size, 16);
    }

    void deallocate(void* ptr)
    {
        _aligned_free(ptr);
    }
};

class PhysicsErrorCallback : public PxErrorCallback
{
public:
    void reportError(PxErrorCode::Enum code, const char* message, const char* file,
        int line)
    {
		std::stringstream ss;
		bool IsInfo = false, IsWarning = false, IsError = false, IsFatal = false;
		switch (code)
		{
			case PxErrorCode::eDEBUG_INFO:
				//! \brief An informational message.
				IsInfo = true;
				ss << "DEBUG_INFO " << message;
				break;
			case PxErrorCode::eDEBUG_WARNING:
				//! \brief a warning message for the user to help with debugging
				IsWarning = true;
				ss << "DEBUG_WARNING " << message;
				break;
			case PxErrorCode::eINVALID_PARAMETER:
				//! \brief method called with invalid parameter(s)
				IsError = true;
				ss << "INVALID_PARAMETER " << message;
				break;
			case PxErrorCode::eINVALID_OPERATION:
				//! \brief method was called at a time when an operation is not possible
				IsError = true;
				ss << "INVALID_OPERATION " << message;
				break;
			case PxErrorCode::eOUT_OF_MEMORY:
				//! \brief method failed to allocate some memory
				IsError = true;
				ss << "OUT_OF_MEMORY " << message;
				break;
			case PxErrorCode::eINTERNAL_ERROR:
				/** \brief The library failed for some reason.
				Possibly you have passed invalid values like NaNs, which are not checked for.
				*/
				IsError = true;
				ss << "INTERNAL_ERROR " << message;
				break;
			case PxErrorCode::eABORT:
				//! \brief An unrecoverable error, execution should be halted and log output flushed
				IsFatal = true;
				ss << "ABORT " << message;
				break;
			case PxErrorCode::ePERF_WARNING:
				//! \brief The SDK has determined that an operation may result in poor performance.
				IsError = true;
				ss << "PERF_WARNING " << message;
				break;
			default:
				//! \brief A bit mask for including all errors
				IsError = true;
				ss << "UNKNOWN_ERROR " << message;
		};

		if (IsInfo)
			log_info(ss.str().c_str(), true);
		else if (IsWarning)
			log_warning(ss.str().c_str(), true);
		else if (IsError)
			log_error(ss.str().c_str(), file, line, true);
		else if (IsFatal)
			log_fatal(0xcc, ss.str().c_str(), file, line);
		else
			log_error(ss.str().c_str(), file, line, true);
    }
};

static PhysicsErrorCallback gDefaultErrorCallback;
static PhysicsHostMemoryAllocator gDefaultAllocatorCallback;

physx::PxFoundation* gFoundation = nullptr;
physx::PxPhysics* gPhysics = nullptr;
physx::PxPvd* gPvd = nullptr;
physx::PxCooking* gCooking = nullptr;
physx::PxDefaultCpuDispatcher* gDispatcher = nullptr;

void Physx_Test();

bool Physx_Initalize()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
	if (!gFoundation)
		return false;
	bool recordMemoryAllocations = true;
	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 150);
	gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), recordMemoryAllocations, gPvd);
	if (!gPhysics)
		return false;

	gCooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, PxCookingParams(PxTolerancesScale()));
	if (!gCooking)
		return false;
	gDispatcher = PxDefaultCpuDispatcherCreate(1);
	Physx_Test();
	return true;
}

PxMaterial* gMaterial;
PxScene* gScene;

PxRigidDynamic* createDynamic(const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity = PxVec3(0))
{
	PxRigidDynamic* dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, 10.0f);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	gScene->addActor(*dynamic);
	return dynamic;
}

void createStack(const PxTransform& t, PxU32 size, PxReal halfExtent)
{
	PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *gMaterial);
	for (PxU32 i = 0; i < size; i++)
	{
		for (PxU32 j = 0; j < size - i; j++)
		{
			PxTransform localTm(PxVec3(PxReal(j * 2) - PxReal(size - i), PxReal(i * 2 + 1), 0) * halfExtent);
			PxRigidDynamic* body = gPhysics->createRigidDynamic(t.transform(localTm));
			body->attachShape(*shape);
			PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
			gScene->addActor(*body);
		}
	}
	shape->release();
}

void Physx_Test()
{
	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0, -4.1, -.4);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	gScene = gPhysics->createScene(sceneDesc);

	gMaterial = gPhysics->createMaterial(5.5f, 0.5f, 0.6f);
	PxRigidStatic* groundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
	gScene->addActor(*groundPlane);
	float stackZ = 69.0;
	for (PxU32 i = 0; i < 5; i++)
		createStack(PxTransform(PxVec3(0, 0, stackZ -= 10.0f)), 10, 2.0f);

	createDynamic(PxTransform(PxVec3(0, 10, 100)), PxSphereGeometry(10), PxVec3(0, -5, -10));
	
	while (1)
	{
		Sleep(1);
		gScene->simulate(1.0f / 30.0);
		gScene->fetchResults(true);
	}
}

void Physx_Destroy() {
	gCooking->release();
	gPhysics->release();
	gPvd->release();
	gFoundation->release();
}

