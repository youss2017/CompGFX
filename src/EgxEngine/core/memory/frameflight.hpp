#pragma once
#include "../egxcommon.hpp"
#include <cassert>

namespace egx
{

    class FrameFlight
    {

    public:
        FrameFlight() = default;
        FrameFlight(const ref<VulkanCoreInterface> &CoreInterface)
        {
            DelayInitalizeFF(CoreInterface);
        }

        void SetStaticFrameIndex(uint32_t FrameIndex = UINT32_MAX)
        {
            assert(_ptr_current_frame && "Cannot set static frame index on single frame objects.");
            _static_frame = FrameIndex;
        }

        void DelayInitalizeFF(const ref<VulkanCoreInterface> &CoreInterface, bool single = false)
        {
            if (single)
            {
                _static_frame = 0;
            }
            else
            {
                _ptr_current_frame = &CoreInterface->CurrentFrame;
                _max_frames = CoreInterface->MaxFramesInFlight;
            }
        }

    protected:
        inline uint32_t GetCurrentFrame() const { return _static_frame == UINT32_MAX ? *_ptr_current_frame : _static_frame; }

    protected:
        uint32_t *_ptr_current_frame = nullptr;
        uint32_t _max_frames = 0;
        uint32_t _static_frame = UINT32_MAX;
    };

}