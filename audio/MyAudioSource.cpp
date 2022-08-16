//
// Created by ksh on 16-Aug-22.
//

#include "MyAudioSource.h"

#include <cmath>
#include <iostream>

//-----------------------------------------------------------

MyAudioSource::~MyAudioSource ()
{
    delete[] pcmAudio;
}
//-----------------------------------------------------------

void MyAudioSource::init ()
{
    pcmAudio = new float[sampleCount];
    const float radsPerSec = 2 * 3.1415926536 * frequency / (float) format.Format.nSamplesPerSec;
    for ( unsigned long i = 0; i < sampleCount; i++ )
    {
        float sampleValue = sin (radsPerSec * (float) i);
        pcmAudio[i] = sampleValue;
    }
    initialised = true;
}
//-----------------------------------------------------------

HRESULT MyAudioSource::SetFormat (WAVEFORMATEX* wfex)
{
    if ( wfex->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
        format = *reinterpret_cast<WAVEFORMATEXTENSIBLE*>( wfex );
    else
    {
        format.Format = *wfex;
        format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        INIT_WAVEFORMATEX_GUID (&format.SubFormat, wfex->wFormatTag);
        format.Samples.wValidBitsPerSample = format.Format.wBitsPerSample;
        format.dwChannelMask = 0;
    }

    const auto formatTag = EXTRACT_WAVEFORMATEX_ID (&format.SubFormat);

    std::cout << "Channel Count: " << format.Format.nChannels << '\n';
    std::cout << "Audio Format: ";
    switch ( formatTag )
    {
        case WAVE_FORMAT_IEEE_FLOAT:
            std::cout << "WAVE_FORMAT_IEEE_FLOAT\n";
            break;
        case WAVE_FORMAT_PCM:
            std::cout << "WAVE_FORMAT_PCM\n";
            break;
        default:
            std::cout << "Wave Format Unknown\n";
            break;
    }
    return 0;
}
//-----------------------------------------------------------

HRESULT MyAudioSource::LoadData (UINT32 totalFrames, BYTE* dataOut, DWORD* flags)
{
    auto* fData = (float*) dataOut;
    UINT32 totalSamples = totalFrames * format.Format.nChannels;
    if ( !initialised )
    {
        init ();
        bufferSize = totalFrames * format.Format.nChannels;
        std::cout << "bufferSize: " << bufferSize << '\n';
        std::cout << "samplesPerChannel: " << totalFrames / format.Format.nChannels << '\n';
        std::cout << "fData[totalSamples]: " << fData[totalFrames] << '\n';
        std::cout << "fData[bufferSize]: " << fData[bufferSize] << '\n';
        std::cout << "buffer address: " << int(dataOut) << '\n';
    }
    else
    {
        std::cout << "Frames to Fill: " << totalFrames << '\n';
        std::cout << "Samples to Fill: " << totalSamples << '\n';
        std::cout << "bufferPos: " << bufferPos << '\n';
        std::cout << "buffer address: " << int(dataOut) << '\n';
    }

    if ( pcmPos < sampleCount )
    {
        for ( UINT32 i = 0; i < totalSamples; i += format.Format.nChannels )
        {
            for ( size_t chan = 0; chan < format.Format.nChannels; ++chan )
                fData[i + chan] = ( pcmPos < sampleCount ) ? pcmAudio[pcmPos] : 0.0f;
            ++pcmPos;
        }
        bufferPos += totalSamples;
        bufferPos %= bufferSize;
    }
    else
        *flags = AUDCLNT_BUFFERFLAGS_SILENT;

    return 0;
}
