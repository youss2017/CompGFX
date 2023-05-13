#include <core/egx.hpp>

namespace egx
{

    class SamplerBuilder
    {
    public:
        SamplerBuilder(const DeviceCtx& ctx);
        void Invalidate();
        vk::Sampler GetSampler() const { return m_Data->m_Sampler; }

        vk::Filter magFilter = (vk::Filter)VK_FILTER_LINEAR;
        vk::Filter minFilter = (vk::Filter)VK_FILTER_LINEAR;
        vk::SamplerMipmapMode mipmapMode = (vk::SamplerMipmapMode)VK_SAMPLER_MIPMAP_MODE_LINEAR;
        vk::SamplerAddressMode addressModeU = (vk::SamplerAddressMode)VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vk::SamplerAddressMode addressModeV = (vk::SamplerAddressMode)VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vk::SamplerAddressMode addressModeW = (vk::SamplerAddressMode)VK_SAMPLER_ADDRESS_MODE_REPEAT;
        float mipLodBias = 0.0;
        bool anisotropyEnable = VK_FALSE;
        float maxAnisotropy = 0.0;
        bool compareEnable = VK_FALSE;
        vk::CompareOp compareOp = (vk::CompareOp)VK_COMPARE_OP_LESS;
        float minLod = 0.0;
        float maxLod = 1000.0;
        vk::BorderColor borderColor = (vk::BorderColor)VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        bool unnormalizedCoordinates = VK_FALSE;

    private:
        struct DataWrapper
        {
            DeviceCtx m_Ctx;
            vk::Sampler m_Sampler;
            void Reinvalidate();
            ~DataWrapper();
        };
        std::shared_ptr<DataWrapper> m_Data;
    };

}