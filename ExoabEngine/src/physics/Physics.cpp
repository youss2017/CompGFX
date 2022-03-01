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
physx::PxScene* gScene = nullptr;

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

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0, -9.81, 0.0);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	gScene = gPhysics->createScene(sceneDesc);

	//Physx_Test();
	return true;
}

static PxMaterial* gMaterial;

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

void Physx_Simulate()
{
	gScene->simulate(1.0 / 60.0);
	gScene->fetchResults(true);
}

physx::PxMaterial* Physx_CreateMaterial(float staticFriction, float dynamicFriction, float restitution)
{
	return gPhysics->createMaterial(staticFriction, dynamicFriction, restitution);
}

physx::PxTriangleMesh* Physx_CreateTriangleMesh(Mesh::Geometry& geometry)
{
	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = geometry.verticesCount;
	meshDesc.points.stride = sizeof(Mesh::GeometryVertex);
	meshDesc.points.data = geometry.m_vertices.data();

	meshDesc.triangles.count = geometry.indicesCount;
	meshDesc.triangles.stride = 3 * sizeof(PxU32);
	meshDesc.triangles.data = geometry.m_indices.data();

	PxDefaultMemoryOutputStream writeBuffer;
	PxTriangleMeshCookingResult::Enum result;
	bool status = gCooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
	if (!status)
		return NULL;

	PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
	return gPhysics->createTriangleMesh(readBuffer);
}

physx::PxShape* Physx_CreateShape(physx::PxGeometry geometry, physx::PxMaterial* material)
{
	PxShape* shape = gPhysics->createShape(geometry, *material);
	return shape;
}

physx::PxRigidDynamic* Physx_CreateDynamicActor(physx::PxMaterial* material, physx::PxTriangleMesh* mesh, glm::vec3 scale, physx::PxTransform pose)
{
	// created earlier
	PxRigidActor* myActor = nullptr;

	// create a shape instancing a triangle mesh at the given scale
	PxMeshScale pscale({ scale.x, scale.y, scale.z }, PxQuat(PxIdentity));
	PxTriangleMeshGeometry geom(mesh, pscale);
	PxRigidDynamic* dynamicActor = gPhysics->createRigidDynamic(pose);
	return dynamicActor;
}
