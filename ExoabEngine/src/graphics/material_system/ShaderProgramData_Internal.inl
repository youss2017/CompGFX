#pragma once
#include <cstdint>
#include <map>
#include "../../core/backend/VkGraphicsCard.hpp"
#include "../../core/memory/Buffer2.hpp"
#include "../../units/Entity.hpp"

struct DynamicUniformInformation
{
    uint32_t bindingID;
    uint32_t offset;
    uint32_t bufferSize;
    uint32_t copyCount;
    uint32_t usedCopies;
    uint32_t startUsedCopies;
    bool FrameChanged = false;
};

struct DynamicSetInformation
{
    uint32_t setHandleIndex;
    VkDescriptorSet setHandle;
    uint32_t setID;
    // bindingID
    std::map<uint32_t, DynamicUniformInformation> uniformInformation;
    IBuffer2 buffer;
    bool isBuffer = false;
};

struct ShaderProgramDataReserved
{
    vk::VkContext context;
    uint32_t lastFrameIndex;
    std::vector<DynamicSetInformation> setInformations;
    VkDescriptorPool descriptorPool;
    uint32_t descriptorSetCount;
    VkDescriptorSet* pDescriptorSets;
};