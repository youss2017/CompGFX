#ifndef DECLARE_POINTER_HEADER
#define DECLARE_POINTER_HEADER

#define DECLARE_POINTER(structure, bufferLayout) layout(buffer_reference, bufferLayout) buffer structure##_T { structure v; }
#define DECLARE_POINTER_ARRAY(structure, bufferLayout) layout(buffer_reference, bufferLayout) buffer structure##_T { structure v[]; }

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : enable

// From: https://community.arm.com/arm-community-blogs/b/graphics-gaming-and-vr-blog/posts/vulkan-buffer-device-address
// Not all devices support 64-bit integers, but it's possible to cast uvec2 <-> pointer.
// Doing address computation like this is fine.
uvec2 uadd_64_32(uvec2 addr, uint offset)
{
    uint carry;
    addr.x = uaddCarry(addr.x, offset, carry);
    addr.y += carry;
    return addr;
}

#endif