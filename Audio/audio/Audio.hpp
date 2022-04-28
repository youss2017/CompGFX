#pragma once
#include <stdint.h>

class Audio {

public:
	Audio();
	~Audio();

	Audio(const Audio& other) = delete;
	Audio(const Audio&& other) = delete;

private:
	void* mNativeInfo;
	friend class AudioBuffer;
};

class AudioBuffer {

public:
	AudioBuffer(const Audio* pAudio, uint32_t sampleRate, float durationInSeconds);
	~AudioBuffer();
	void Play();
	// int(sampleRate * (2 channels) * durationInSeconds)
	float* GetBuffer() { return mAudioData; }
private:
	uint32_t mSampleRate;
	float mDuration;
	void* mNativeInfo;
	float* mAudioData;
};
