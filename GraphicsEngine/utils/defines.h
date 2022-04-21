#pragma once
#include <cstdint>
#include <malloc.h>

// Only for Visual Studio
// TODO: Test performace between _malloca and the normal alloca
// _malloca is safer since it would allocate from heap if there is not enough space on the stack.
#if _MSC_VER >= 1900
#define stack_allocate(size) alloca(size) //_malloca(size)
#define stack_free(size) //_freea(size)
#else
#define stack_allocate(size) alloca(size)
#define stack_free(size)
#endif

// The following are used to prevent compiler warnings.
#define UInt32ToPVOID(x) ((void*)((uint64_t)x))
#define PVOIDToUInt32(x) ((uint32_t)((uint64_t)x))
#define APIToUInt32(x) PVOIDToUInt32(x)
