#pragma once
#include "../egxcommon.hpp"
#include <cassert>

namespace egx
{

	class FrameFlight
	{

	public:
		FrameFlight() = default;
		FrameFlight(const ref<VulkanCoreInterface>& CoreInterface)
		{
			DelayInitalizeFF(CoreInterface);
		}

		void SetStaticFrameIndex(uint32_t FrameIndex = UINT32_MAX)
		{
			assert(_ptr_current_frame && "Cannot set static frame index on single frame objects.");
			_static_frame = FrameIndex;
		}

	protected:
		void DelayInitalizeFF(const ref<VulkanCoreInterface>& CoreInterface, bool single = false)
		{
			if (single)
			{
				_static_frame = 0;
			}
			else
			{
				_max_frames = CoreInterface->MaxFramesInFlight;
			}
			_ptr_current_frame = &CoreInterface->CurrentFrame;
			_ptr_frame_count = &CoreInterface->FrameCount;
		}

		inline uint32_t GetCurrentFrame() const { return _static_frame == UINT32_MAX ? *_ptr_current_frame : _static_frame; }

	protected:
		uint32_t* _ptr_current_frame = nullptr;
		size_t* _ptr_frame_count = nullptr;
		uint32_t _max_frames = 1;
		uint32_t _static_frame = UINT32_MAX;
	};

}