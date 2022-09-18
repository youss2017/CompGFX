#pragma once

typedef unsigned int EventFlagBits;
typedef unsigned int EventDetailBits;

extern "C" {

enum EventFlags {
	EVENT_FLAGS_MOUSE_PRESS   = 0x000001,
	EVENT_FLAGS_MOUSE_RELEASE = 0x000010,
	EVENT_FLAGS_MOUSE_MOVE	= 0x000100,
	EVENT_FLAGS_KEY_PRESS     = 0x001000, 
	EVENT_FLAGS_KEY_RELEASE	= 0x010000,
	EVENT_FLAGS_WINDOW_RESIZE = 0x100000
};

enum EventDetailFlags {
	EVENT_DETAIL_LEFT_BUTTON,
	EVENT_DETAIL_RIGHT_BUTTON,
	EVENT_DETAIL_MIDDLE_BUTTON,
	EVENT_DETAIL_NORMAL_MOTION,
	EVENT_DETAIL_RAW_MOTION,
	EVENT_DETAIL_UNDEFINED = 0x7fffffff
};


	struct Event {
		EventFlagBits mEvents;
		EventDetailBits mDetails;

		struct {
			int mClickX;
			int mClickY;
			union {
				struct {
					int mMotionX;
					int mMotionY;
				};
				struct {
					int mPositionX;
					int mPositionY;
				};
				struct {
					int mWidth;
					int mHeight;
				};
			};
			int NonASCIKey;
			char KeyLowerCase;
			char KeyUpperCase;
		} mPayload;
	};

}
