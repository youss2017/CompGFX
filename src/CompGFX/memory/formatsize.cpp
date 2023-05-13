#include "formatsize.hpp"
#include <map>
#include <cassert>

static std::map<VkFormat, uint32_t> mappings;

uint32_t egx::FormatByteCount(VkFormat format)
{
    uint32_t check = (uint32_t)format;
    assert(check > 0 && check <= 130 && "You either entered a invalid format or the format is not supported (compression formats not supported).");
    mappings.insert({VK_FORMAT_R4G4_UNORM_PACK8, 1});
    mappings.insert({VK_FORMAT_R4G4B4A4_UNORM_PACK16, 2});
    mappings.insert({VK_FORMAT_B4G4R4A4_UNORM_PACK16, 2});
    mappings.insert({VK_FORMAT_R5G6B5_UNORM_PACK16, 2});
    mappings.insert({VK_FORMAT_B5G6R5_UNORM_PACK16, 2});
    mappings.insert({VK_FORMAT_R5G5B5A1_UNORM_PACK16, 2});
    mappings.insert({VK_FORMAT_B5G5R5A1_UNORM_PACK16, 2});
    mappings.insert({VK_FORMAT_A1R5G5B5_UNORM_PACK16, 2});
    mappings.insert({VK_FORMAT_R8_UNORM, 1});
    mappings.insert({VK_FORMAT_R8_SNORM, 1});
    mappings.insert({VK_FORMAT_R8_USCALED, 1});
    mappings.insert({VK_FORMAT_R8_SSCALED, 1});
    mappings.insert({VK_FORMAT_R8_UINT, 1});
    mappings.insert({VK_FORMAT_R8_SINT, 1});
    mappings.insert({VK_FORMAT_R8_SRGB, 1});
    mappings.insert({VK_FORMAT_R8G8_UNORM, 2});
    mappings.insert({VK_FORMAT_R8G8_SNORM, 2});
    mappings.insert({VK_FORMAT_R8G8_USCALED, 2});
    mappings.insert({VK_FORMAT_R8G8_SSCALED, 2});
    mappings.insert({VK_FORMAT_R8G8_UINT, 2});
    mappings.insert({VK_FORMAT_R8G8_SINT, 2});
    mappings.insert({VK_FORMAT_R8G8_SRGB, 2});
    mappings.insert({VK_FORMAT_R8G8B8_UNORM, 3});
    mappings.insert({VK_FORMAT_R8G8B8_SNORM, 3});
    mappings.insert({VK_FORMAT_R8G8B8_USCALED, 3});
    mappings.insert({VK_FORMAT_R8G8B8_SSCALED, 3});
    mappings.insert({VK_FORMAT_R8G8B8_UINT, 3});
    mappings.insert({VK_FORMAT_R8G8B8_SINT, 3});
    mappings.insert({VK_FORMAT_R8G8B8_SRGB, 3});
    mappings.insert({VK_FORMAT_B8G8R8_UNORM, 3});
    mappings.insert({VK_FORMAT_B8G8R8_SNORM, 3});
    mappings.insert({VK_FORMAT_B8G8R8_USCALED, 3});
    mappings.insert({VK_FORMAT_B8G8R8_SSCALED, 3});
    mappings.insert({VK_FORMAT_B8G8R8_UINT, 3});
    mappings.insert({VK_FORMAT_B8G8R8_SINT, 3});
    mappings.insert({VK_FORMAT_B8G8R8_SRGB, 3});
    mappings.insert({VK_FORMAT_R8G8B8A8_UNORM, 4});
    mappings.insert({VK_FORMAT_R8G8B8A8_SNORM, 4});
    mappings.insert({VK_FORMAT_R8G8B8A8_USCALED, 4});
    mappings.insert({VK_FORMAT_R8G8B8A8_SSCALED, 4});
    mappings.insert({VK_FORMAT_R8G8B8A8_UINT, 4});
    mappings.insert({VK_FORMAT_R8G8B8A8_SINT, 4});
    mappings.insert({VK_FORMAT_R8G8B8A8_SRGB, 4});
    mappings.insert({VK_FORMAT_B8G8R8A8_UNORM, 4});
    mappings.insert({VK_FORMAT_B8G8R8A8_SNORM, 4});
    mappings.insert({VK_FORMAT_B8G8R8A8_USCALED, 4});
    mappings.insert({VK_FORMAT_B8G8R8A8_SSCALED, 4});
    mappings.insert({VK_FORMAT_B8G8R8A8_UINT, 4});
    mappings.insert({VK_FORMAT_B8G8R8A8_SINT, 4});
    mappings.insert({VK_FORMAT_B8G8R8A8_SRGB, 4});
    mappings.insert({VK_FORMAT_A8B8G8R8_UNORM_PACK32, 4});
    mappings.insert({VK_FORMAT_A8B8G8R8_SNORM_PACK32, 4});
    mappings.insert({VK_FORMAT_A8B8G8R8_USCALED_PACK32, 4});
    mappings.insert({VK_FORMAT_A8B8G8R8_SSCALED_PACK32, 4});
    mappings.insert({VK_FORMAT_A8B8G8R8_UINT_PACK32, 4});
    mappings.insert({VK_FORMAT_A8B8G8R8_SINT_PACK32, 4});
    mappings.insert({VK_FORMAT_A8B8G8R8_SRGB_PACK32, 4});
    mappings.insert({VK_FORMAT_A2R10G10B10_UNORM_PACK32, 4});
    mappings.insert({VK_FORMAT_A2R10G10B10_SNORM_PACK32, 4});
    mappings.insert({VK_FORMAT_A2R10G10B10_USCALED_PACK32, 4});
    mappings.insert({VK_FORMAT_A2R10G10B10_SSCALED_PACK32, 4});
    mappings.insert({VK_FORMAT_A2R10G10B10_UINT_PACK32, 4});
    mappings.insert({VK_FORMAT_A2R10G10B10_SINT_PACK32, 4});
    mappings.insert({VK_FORMAT_A2B10G10R10_UNORM_PACK32, 4});
    mappings.insert({VK_FORMAT_A2B10G10R10_SNORM_PACK32, 4});
    mappings.insert({VK_FORMAT_A2B10G10R10_USCALED_PACK32, 4});
    mappings.insert({VK_FORMAT_A2B10G10R10_SSCALED_PACK32, 4});
    mappings.insert({VK_FORMAT_A2B10G10R10_UINT_PACK32, 4});
    mappings.insert({VK_FORMAT_A2B10G10R10_SINT_PACK32, 4});
    mappings.insert({VK_FORMAT_R16_UNORM, 2});
    mappings.insert({VK_FORMAT_R16_SNORM, 2});
    mappings.insert({VK_FORMAT_R16_USCALED, 2});
    mappings.insert({VK_FORMAT_R16_SSCALED, 2});
    mappings.insert({VK_FORMAT_R16_UINT, 2});
    mappings.insert({VK_FORMAT_R16_SINT, 2});
    mappings.insert({VK_FORMAT_R16_SFLOAT, 2});
    mappings.insert({VK_FORMAT_R16G16_UNORM, 4});
    mappings.insert({VK_FORMAT_R16G16_SNORM, 4});
    mappings.insert({VK_FORMAT_R16G16_USCALED, 4});
    mappings.insert({VK_FORMAT_R16G16_SSCALED, 4});
    mappings.insert({VK_FORMAT_R16G16_UINT, 4});
    mappings.insert({VK_FORMAT_R16G16_SINT, 4});
    mappings.insert({VK_FORMAT_R16G16_SFLOAT, 4});
    mappings.insert({VK_FORMAT_R16G16B16_UNORM, 6});
    mappings.insert({VK_FORMAT_R16G16B16_SNORM, 6});
    mappings.insert({VK_FORMAT_R16G16B16_USCALED, 6});
    mappings.insert({VK_FORMAT_R16G16B16_SSCALED, 6});
    mappings.insert({VK_FORMAT_R16G16B16_UINT, 6});
    mappings.insert({VK_FORMAT_R16G16B16_SINT, 6});
    mappings.insert({VK_FORMAT_R16G16B16_SFLOAT, 6});
    mappings.insert({VK_FORMAT_R16G16B16A16_UNORM, 8});
    mappings.insert({VK_FORMAT_R16G16B16A16_SNORM, 8});
    mappings.insert({VK_FORMAT_R16G16B16A16_USCALED, 8});
    mappings.insert({VK_FORMAT_R16G16B16A16_SSCALED, 8});
    mappings.insert({VK_FORMAT_R16G16B16A16_UINT, 8});
    mappings.insert({VK_FORMAT_R16G16B16A16_SINT, 8});
    mappings.insert({VK_FORMAT_R16G16B16A16_SFLOAT, 8});
    mappings.insert({VK_FORMAT_R32_UINT, 4});
    mappings.insert({VK_FORMAT_R32_SINT, 4});
    mappings.insert({VK_FORMAT_R32_SFLOAT, 4});
    mappings.insert({VK_FORMAT_R32G32_UINT, 8});
    mappings.insert({VK_FORMAT_R32G32_SINT, 8});
    mappings.insert({VK_FORMAT_R32G32_SFLOAT, 8});
    mappings.insert({VK_FORMAT_R32G32B32_UINT, 12});
    mappings.insert({VK_FORMAT_R32G32B32_SINT, 12});
    mappings.insert({VK_FORMAT_R32G32B32_SFLOAT, 12});
    mappings.insert({VK_FORMAT_R32G32B32A32_UINT, 16});
    mappings.insert({VK_FORMAT_R32G32B32A32_SINT, 16});
    mappings.insert({VK_FORMAT_R32G32B32A32_SFLOAT, 16});
    mappings.insert({VK_FORMAT_R64_UINT, 8});
    mappings.insert({VK_FORMAT_R64_SINT, 8});
    mappings.insert({VK_FORMAT_R64_SFLOAT, 8});
    mappings.insert({VK_FORMAT_R64G64_UINT, 16});
    mappings.insert({VK_FORMAT_R64G64_SINT, 16});
    mappings.insert({VK_FORMAT_R64G64_SFLOAT, 16});
    mappings.insert({VK_FORMAT_R64G64B64_UINT, 24});
    mappings.insert({VK_FORMAT_R64G64B64_SINT, 24});
    mappings.insert({VK_FORMAT_R64G64B64_SFLOAT, 24});
    mappings.insert({VK_FORMAT_R64G64B64A64_UINT, 32});
    mappings.insert({VK_FORMAT_R64G64B64A64_SINT, 32});
    mappings.insert({VK_FORMAT_R64G64B64A64_SFLOAT, 32});
    mappings.insert({VK_FORMAT_B10G11R11_UFLOAT_PACK32, 4});
    mappings.insert({VK_FORMAT_E5B9G9R9_UFLOAT_PACK32, 4});
    mappings.insert({VK_FORMAT_D16_UNORM, 2});
    mappings.insert({VK_FORMAT_X8_D24_UNORM_PACK32, 4});
    mappings.insert({VK_FORMAT_D32_SFLOAT, 4});
    mappings.insert({VK_FORMAT_S8_UINT, 1});
    mappings.insert({VK_FORMAT_D16_UNORM_S8_UINT, 3});
    mappings.insert({VK_FORMAT_D24_UNORM_S8_UINT, 4});
    mappings.insert({VK_FORMAT_D32_SFLOAT_S8_UINT, 5});
    return mappings[format];
}

vk::ImageAspectFlags egx::GetFormatAspectFlags(vk::Format format)
{
    vk::ImageAspectFlags aspectFlags;

    if ((format == vk::Format::eD16Unorm) || (format == vk::Format::eD32Sfloat))
    {
        aspectFlags |= vk::ImageAspectFlagBits::eDepth;
    }
    else if (format == vk::Format::eS8Uint)
    {
        aspectFlags |= vk::ImageAspectFlagBits::eStencil;
    }
    else if ((format == vk::Format::eD16UnormS8Uint) || (format == vk::Format::eD24UnormS8Uint) || (format == vk::Format::eD32SfloatS8Uint))
    {
        aspectFlags |= vk::ImageAspectFlagBits::eDepth;
        aspectFlags |= vk::ImageAspectFlagBits::eStencil;
    }
    else // Assume color format
    {
        aspectFlags |= vk::ImageAspectFlagBits::eColor;
    }

    return aspectFlags;
}
