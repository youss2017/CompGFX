#include "PhysicsMain.hpp"
#include <vehicle/PxVehicleSDK.h>
#include <foundation/PxAllocatorCallback.h>
#include <foundation/PxErrorCallback.h>
#include "../utils/Logger.hpp"
#include <string>
#include <sstream>
#include <cassert>
// To specifiy directory of dlls
#include <Windows.h>
#include <iostream>

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

PhysicsMain::PhysicsMain()
{
	m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
	assert(m_foundation);
	bool recordMemoryAllocations = true;
	m_visual_debugger = PxCreatePvd(*m_foundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 150);
	m_visual_debugger->connect(*transport, PxPvdInstrumentationFlag::eALL);

	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, PxTolerancesScale(), recordMemoryAllocations, m_visual_debugger);
	assert(m_physics);

	exit(0);
}

PhysicsMain::~PhysicsMain()
{
}
