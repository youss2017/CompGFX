#if defined(_WIN32)
#include "Audio.hpp"
#include <Windows.h>
#include <xaudio2.h>
#include <string>

struct Win32Audio {
	IXAudio2* pAudioDevice;
	IXAudio2MasteringVoice* pMasterVoice;
};

Audio::Audio() {
	Win32Audio* aud = new Win32Audio();
	XAudio2Create(&aud->pAudioDevice);
	aud->pAudioDevice->CreateMasteringVoice(&aud->pMasterVoice, 2, 44100, 0, NULL, nullptr, AudioCategory_GameMedia);
	aud->pAudioDevice->StartEngine();
	mNativeInfo = aud;
}

Audio::~Audio() {
	Win32Audio* aud = (Win32Audio*)mNativeInfo;
	aud->pAudioDevice->StopEngine();
	aud->pMasterVoice->DestroyVoice();
	delete mNativeInfo;
}

AudioBuffer::AudioBuffer(const Audio* _pAudio, uint32_t sampleRate, float durationInSeconds) : mSampleRate(sampleRate), mDuration(durationInSeconds) {
	IXAudio2* pAudio = ((Win32Audio*)_pAudio->mNativeInfo)->pAudioDevice;
	IXAudio2SourceVoice* pSource;
	WAVEFORMATEX wfx;
	wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;        /* format type */
	wfx.nChannels = 2;         /* number of channels (i.e. mono, stereo...) */
	wfx.nSamplesPerSec = sampleRate;    /* sample rate */
	wfx.nAvgBytesPerSec = 2 * (sizeof(float) * sampleRate);   /* for buffer estimation */
	wfx.nBlockAlign = sizeof(float) * 2;       /* block size of data */
	wfx.wBitsPerSample = 32;    /* Number of bits per sample of mono data */
	wfx.cbSize = 0;            /* The count in bytes of the size of
									extra information (after cbSize) */
	pAudio->CreateSourceVoice(&pSource, &wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr);
	pSource->Start();
	mNativeInfo = pSource;
	mAudioData = new float[int(float(sampleRate) * 2.0 * durationInSeconds)];
	memset(mAudioData, 0, int(float(sampleRate) * 2.0 * durationInSeconds) * 4);
}

AudioBuffer::~AudioBuffer() {
	((IXAudio2SourceVoice*)mNativeInfo)->Stop();
	((IXAudio2SourceVoice*)mNativeInfo)->DestroyVoice();
	delete[] mAudioData;

}

void AudioBuffer::Play() {
	XAUDIO2_BUFFER buffer{};
	buffer.Flags = 0;                       // Either 0 or XAUDIO2_END_OF_STREAM.
	buffer.AudioBytes = int(float(mSampleRate) * 4.0 * mDuration * 2.0);                  // Size of the audio data buffer in bytes.
	buffer.pAudioData = (BYTE*)mAudioData;             // Pointer to the audio data buffer.
	buffer.PlayBegin = 0;                   // First sample in this buffer to be played.
	buffer.PlayLength = 0;                  // Length of the region to be played in samples,
										//  or 0 to play the whole buffer.
	((IXAudio2SourceVoice*)mNativeInfo)->SubmitSourceBuffer(&buffer);
}
#endif
