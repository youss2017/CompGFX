#include "PhysicsEngine.hpp"
#include "Globals.hpp"
#include "PxPhysicsAPI.h"
#include "extensions/PxDefaultAllocator.h"
#include "pvd/PxPvd.h"
#include "Globals.hpp"
#include "extensions/PxDefaultSimulationFilterShader.h"

// F = ma
// F : N --> (kg * m) / s^2
// m : kg
// a : m / s^2

namespace Ph {

	static physx::PxDefaultErrorCallback gDefaultErrorCallback;
	static physx::PxDefaultAllocator gDefaultAllocatorCallback;

	PhysicsEngine::PhysicsEngine(float Cube1x1MeterPixelScale, vec3 gravity)
	{
		mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
		mScale.length = 100;
		mScale.speed = 981;
		bool trackMemoryAllocation = false;
#if defined(_DEBUG)
		mPVD = physx::PxCreatePvd(*mFoundation);
		const char* PVD_HOST = "127.0.0.1";
		physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
		mPVD->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
		trackMemoryAllocation = true;
#endif
		mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, mScale, trackMemoryAllocation, mPVD);
		physx::PxCookingParams cookParameters(mScale);
		mCooking = PxCreateCooking(PX_PHYSICS_VERSION, *mFoundation, cookParameters);
		physx::PxSceneDesc desc(mScale);
		desc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
		mCpuDispatcher = physx::PxDefaultCpuDispatcherCreate(1);
		desc.cpuDispatcher = mCpuDispatcher;
		desc.filterShader = physx::PxDefaultSimulationFilterShader;
		mMasterScene = mPhysics->createScene(desc);
	}

	PhysicsEngine::~PhysicsEngine()
	{
		for (auto material : vMaterials)
			material->release();
		mCooking->release();
		mMasterScene->release();
		mPVD->release();
		mCpuDispatcher->release();
		mPhysics->release();
		mFoundation->release();
	}

	int PhysicsEngine::CreateMaterial(float staticFriction, float dynamicFriction, float restitution)
	{
		auto m = mPhysics->createMaterial(staticFriction, dynamicFriction, restitution);
		if (m == nullptr)
			return -1;
		vMaterials.push_back(m);
		return vMaterials.size() - 1;
	}

	PhysicsEntity* PhysicsEngine::CreateRigidEntity(ecs::IEntityGeometry eg, const ShaderTypes::InstanceData& data, int nMaterialID, bool bDynamic) {
		assert(nMaterialID >= 0 && nMaterialID < vMaterials.size() && "Material ID Out of Bounds");
		const Mesh::Geometry& geo = Global::Geomtry[eg->m_geometryID];
		auto rotation = ExtractRotation(*(glm::mat4*)&data.mModel);
		auto pos = Ph::ExtractPosition(data.mModel);
		physx::PxTransform transform = physx::PxTransform(pos.x, pos.y, pos.z, *(physx::PxQuat*)&rotation);
		physx::PxRigidActor* actor;
		if (bDynamic) {
			actor = mPhysics->createRigidDynamic(transform);
		}
		else {
			actor = mPhysics->createRigidStatic(transform);
		}
		physx::PxSphereGeometry BoundingSphere;
		BoundingSphere.radius = geo.m_bounding_sphere_radius;
		auto shape = physx::PxRigidActorExt::createExclusiveShape(*actor, BoundingSphere, *vMaterials[nMaterialID]);
		mMasterScene->addActor(*actor);
		physx::PxRigidBodyExt::setMassAndUpdateInertia(*(physx::PxRigidDynamic*)actor, 850);
		PhysicsEntity* e = new PhysicsEntity(bDynamic, shape, actor);
		e->pEG = eg;
		e->sData = data;
		e->mBoundingSphere = BoundingSphere;
		eg->AddEntity(e);
		return e;
	}

	void PhysicsEngine::DestroyRigidEntity(PhysicsEntity* e) {
		e->pEG->RemoveEntity(e);
		delete e;
	}

	void PhysicsEngine::AddPlane(vec4 plane, int nMaterialID) {
		auto p = physx::PxCreatePlane(*mPhysics, *(physx::PxPlane*)&plane, *vMaterials[nMaterialID]);
		mMasterScene->addActor(*p);
		vPlanes.push_back(p);
	}

	void PhysicsEngine::SetTerrain(Terrain* terrain) {
		using namespace physx;
		int sampleCount = terrain->GetIndicesBuffer()->mSize / sizeof(int);
		PxHeightFieldSample* samples = new PxHeightFieldSample[sampleCount];
		for (int i = 0, q = 0; i < terrain->GetSubmeshCount(); i++) {
			auto& submesh = terrain->GetSubmesh(i);
			for (int j = 0; j < submesh.mIndicesCount; j++) {
				vec3 pos = FullVec3(terrain->mVertices[submesh.mFirstVertex + terrain->mIndices[submesh.mFirstIndex + j]].inPosition16);
				auto& sample = samples[q++];
				sample.height = pos.y;
			}
		}

#if 0
		PxHeightFieldDesc hfDesc;
		hfDesc.format = PxHeightFieldFormat::eS16_TM;
		hfDesc.nbColumns = numCols;
		hfDesc.nbRows = numRows;
		hfDesc.samples.data = samples;
		hfDesc.samples.stride = sizeof(PxHeightFieldSample);

		PxHeightField* aHeightField = theCooking->createHeightField(hfDesc,
			thePhysics->getPhysicsInsertionCallback());

		PxHeightFieldGeometry hfGeom(aHeightField, PxMeshGeometryFlags(), heightScale, rowScale,
			colScale);
		PxShape* aHeightFieldShape = PxRigidActorExt::createExclusiveShape(*aHeightFieldActor,
			hfGeom, aMaterialArray, nbMaterials);
#endif

		delete[] samples;
	}

	void PhysicsEngine::Start()
	{
		float dTime = Global::Time;
		mMasterScene->simulate(1.0 / 30.0);
	}

	void PhysicsEngine::End() {
		mMasterScene->fetchResults(true);
	}

	bool BBPlaneIntersect(const Plane& plane, const BoundingBox& box)
	{
		vec3 points[8];
		points[0] = vec3(box.mBoxMin.x, box.mBoxMin.y, box.mBoxMin.z);
		points[1] = vec3(box.mBoxMax.x, box.mBoxMin.y, box.mBoxMin.z);
		points[2] = vec3(box.mBoxMax.x, box.mBoxMin.y, box.mBoxMax.z);
		points[3] = vec3(box.mBoxMin.x, box.mBoxMin.y, box.mBoxMax.z);
		points[4] = vec3(box.mBoxMax.x, box.mBoxMax.y, box.mBoxMax.z);
		points[5] = vec3(box.mBoxMin.x, box.mBoxMax.y, box.mBoxMax.z);
		points[6] = vec3(box.mBoxMin.x, box.mBoxMax.y, box.mBoxMin.z);
		points[7] = vec3(box.mBoxMax.x, box.mBoxMax.y, box.mBoxMin.z);
		for (int i = 0; i < 8; i++)
			if (dot(plane.mNormal, points[i]) + plane.mD < 0)
				return true;
		return false;
	}


	void PhysicsEntity::Update() {
		physx::PxTransform t = mActor->getGlobalPose();
		sData.mModel = translate(mat4(1.0), vec3(t.p.x, t.p.y, t.p.z)) * QuatToMatrix(*(quat*)&t.q);
		sData.mNormalModel = inverse(sData.mModel);
	}

	PhysicsEntity::~PhysicsEntity() {
		this->mSphereShape->release();
	}


}