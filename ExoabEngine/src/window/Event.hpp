#pragma once

typedef unsigned int EventFlagBits;
typedef unsigned int EventDetailBits;

enum EventFlags {
	EVENT_MOUSE_PRESS   = 0x000001,
	EVENT_MOUSE_RELEASE = 0x000010,
	EVENT_KEY_PRESS     = 0x000100, 
	EVENT_KEY_RELEASE	= 0x001000, 
};

enum EventDetailFlags {
	EVENT_DETAIL_LEFT_BUTTON,
	EVENT_DETAIL_RIGHT_BUTTON,
	EVENT_DETAIL_MIDDLE_BUTTON,
	EVENT_DETAIL_UNDEFINED = 0x7fffffff
};

struct Event {
	EventFlagBits mEvents;
	EventDetailBits mDetails;

	struct {
		int mClickX;
		int mClickY;
		int mMotionX;
		int mMotionY;
		int NonASCIKey;
		char KeyLowerCase;
		char KeyUpperCase;
	} mPayload;
};
