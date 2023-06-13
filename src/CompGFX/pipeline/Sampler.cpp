#include "Sampler.hpp"
using namespace std;
using namespace egx;
using namespace vk;

egx::ISamplerBuilder::ISamplerBuilder(const DeviceCtx &ctx)
{
    m_Data = make_shared<ISamplerBuilder::DataWrapper>();
    m_Data->m_Ctx = ctx;
}

void egx::ISamplerBuilder::Invalidate()
{
    m_Data->Reinvalidate();
    SamplerCreateInfo createInfo;
    createInfo.magFilter = magFilter;
    createInfo.minFilter = minFilter;
    createInfo.mipmapMode = mipmapMode;
    createInfo.addressModeU = addressModeU;
    createInfo.addressModeV = addressModeV;
    createInfo.addressModeW = addressModeW;
    createInfo.mipLodBias = mipLodBias;
    createInfo.anisotropyEnable = anisotropyEnable;
    createInfo.maxAnisotropy = maxAnisotropy;
    createInfo.compareEnable = compareEnable;
    createInfo.compareOp = compareOp;
    createInfo.minLod = minLod;
    createInfo.maxLod = maxLod;
    createInfo.borderColor = borderColor;
    createInfo.unnormalizedCoordinates = unnormalizedCoordinates;
    m_Data->m_Sampler = m_Data->m_Ctx->Device.createSampler(createInfo);
}

void egx::ISamplerBuilder::DataWrapper::Reinvalidate()
{
    if(m_Ctx && m_Sampler) {
        m_Ctx->Device.destroySampler(m_Sampler);
        m_Sampler = nullptr;
    }
}

egx::ISamplerBuilder::DataWrapper::~DataWrapper()
{
    Reinvalidate();
}