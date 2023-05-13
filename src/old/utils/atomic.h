#pragma once
#if defined(_WIN32)
#include <Windows.h>

// Returns the result of the addition
#define atomicAdd(target, value) (InterlockedAdd(&target, value))
// Returns the result of the increment
#define atomicIncrement(target) (InterlockedIncrement(&target))
// Returns the result of the substraction
#define atomicSub(target, value) atomicAdd(target, -value)
// Returns the result of the target-1
#define atomicDecrement(target) (InterlockedDecrement(&target))

// Returns the result of the addition
#define atomicAdd16(target, value) (InterlockedAdd16(&target, value))
// Returns the result of the increment
#define atomicIncrement16(target) (InterlockedIncrement16(&target))
// Returns the result of the substraction
#define atomicSub16(target, value) atomicAdd16(target, -value)
// Returns the result of the target-1
#define atomicDecrement16(target) (InterlockedDecrement16(&target))

// Returns the result of the addition
#define atomicAdd64(target, value) (InterlockedAdd64(&target, value))
// Returns the result of the increment
#define atomicIncrement64(target) (InterlockedIncrement64(&target))
// Returns the result of the substraction
#define atomicSub64(target, value) atomicAdd64(target, -value)
// Returns the result of the decrement
#define atomicDecrement64(target) (InterlockedDecrement64(&target))

// Returns the original value of target
#define atomicSet(target, value) (InterlockedExchange(&target, value))
// Returns value of target
#define atomicRead(target) (InterlockedExchange(&target, target))

// Returns the original value of target
#define atomicSet16(target, value) (InterlockedExchange16(&target, value))
// Returns value of target
#define atomicRead16(target) (InterlockedExchange16(&target, target))

// Returns the original value of target
#define atomicSet64(target, value) (InterlockedExchange64(&target, value))
// Returns value of target
#define atomicRead64(target) (InterlockedExchange64(&target, target))

// Returns original value of target
#define atomicAnd(target, value) (InterlockedAnd(&target, value))
// Returns original value of target
#define atomicAnd16(target, value) (InterlockedAnd16(&target, value))
// Returns original value of target
#define atomicAnd64(target, value) (InterlockedAnd64(&target, value))

// Returns original value of target
#define atomicOr(target, value) (InterlockedOr(&target, value))
// Returns original value of target
#define atomicOr16(target, value) (InterlockedOr16(&target, value))
// Returns original value of target
#define atomicOr64(target, value) (InterlockedOr64(&target, value))

// Returns original value of target
#define atomicXor(target, value) (InterlockedXor(&target, value))
// Returns original value of target
#define atomicXor16(target, value) (InterlockedXor16(&target, value))
// Returns original value of target
#define atomicXor64(target, value) (InterlockedXor64(&target, value))

#else
#error "Platform is not supported yet"
#endif