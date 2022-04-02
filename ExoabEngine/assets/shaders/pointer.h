#ifndef DECLARE_POINTER_HEADER
#define DECLARE_POINTER_HEADER

#define DECLARE_POINTER(structure, bufferLayout) layout(buffer_reference, bufferLayout) buffer structure##_T { structure v; }
#define DECLARE_POINTER_ARRAY(structure, bufferLayout) layout(buffer_reference, bufferLayout) buffer structure##_T { structure v[]; }
#define sizeof(Type) (uint64_t(Type(uint64_t(0))+1))

#extension GL_EXT_buffer_reference : require
//#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_EXT_scalar_block_layout : enable

#endif