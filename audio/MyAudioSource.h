#pragma once

#include <audioclient.h>

class MyAudioSource
{
public:
    MyAudioSource () = default;
    explicit MyAudioSource (float freq) : frequency(freq) {};
    ~MyAudioSource ();

    HRESULT SetFormat (WAVEFORMATEX*);
    HRESULT LoadData (UINT32, BYTE*, DWORD*);

private:
    void init ();
    bool initialised = false;
    WAVEFORMATEXTENSIBLE format{};
    unsigned int pcmPos = 0;
    UINT32 bufferSize = 0;
    UINT32 bufferPos = 0;
    static const unsigned int sampleCount = 96000 * 5;
    float frequency = 440;
    float* pcmAudio = nullptr;
};
