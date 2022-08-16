#pragma once

#include <audioclient.h>
#include <fstream>

class MyAudioSink
{
public:
    MyAudioSink();
    ~MyAudioSink();

    HRESULT SetFormat(WAVEFORMATEX* pwfx);
    HRESULT CopyData(BYTE* pData, UINT32 numFramesAvailable, BOOL* bDone);

private:
    HRESULT _Initialize_File();
    HRESULT _File_WrapUp();

private:
    bool bComplete;

    size_t data_chunk_pos;
    size_t file_length;
    std::ofstream mainFile;

    //sample format
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    WORD  cbSize;
    int test;
};
