#include "MyAudioSink.h"

#include <Mmdeviceapi.h>
#include <iostream>

namespace little_endian_io
{
    template<typename Word>
    std::ostream &write_word(std::ostream &outs, Word value, unsigned size = sizeof(Word))
    {
        for (; size; -- size, value >>= 8)
            outs.put(static_cast <char> (value & 0xFF));
        return outs;
    }
}
using namespace little_endian_io;

MyAudioSink::MyAudioSink()
{
    _Initialize_File();
}

MyAudioSink::~MyAudioSink()
{
    _File_WrapUp();
}

HRESULT MyAudioSink::_Initialize_File()
{
    std::cout << "initializing file" << std::endl;

    // prepare our wav file
    mainFile.open("example.wav", std::ios::out | std::ios::binary);

    // Write the file headers and sound format
    mainFile << "RIFF----WAVEfmt ";     // (chunk size to be filled in later)
    write_word(mainFile, 16, 4);  // no extension data
    write_word(mainFile, 1, 2);  // PCM - integer samples
    write_word(mainFile, nChannels, 2);  // two channels (stereo file)
    write_word(mainFile, nSamplesPerSec, 4);  // samples per second (Hz)
    write_word(mainFile, nAvgBytesPerSec, 4);  // (Sample Rate * BitsPerSample * Channels) / 8
    write_word(mainFile, nBlockAlign, 2);  // data block size (size of two integer samples, one for each channel, in bytes)
    write_word(mainFile, wBitsPerSample, 2);  // number of bits per sample (use a multiple of 8)

    // Write the data chunk header
    data_chunk_pos = mainFile.tellp();
    mainFile << "data----";  // (chunk size to be filled in later)..

    //start by setting our complete variable to False, main func will turn to true
    bComplete = false;
    //testing
    test = 0;

    return S_OK;
}

HRESULT MyAudioSink::SetFormat(WAVEFORMATEX *pwfx)
{
    //Update our format variables
    wFormatTag = pwfx->wFormatTag;
    nChannels = pwfx->nChannels;
    nSamplesPerSec = pwfx->nSamplesPerSec;
    nAvgBytesPerSec = pwfx->nAvgBytesPerSec;
    nBlockAlign = pwfx->nBlockAlign;
    wBitsPerSample = pwfx->wBitsPerSample;
    cbSize = pwfx->cbSize;

    return S_OK;
}

HRESULT MyAudioSink::CopyData(BYTE *pData, UINT32 numFramesAvailable, BOOL *bDone)
{
    //forgot how to do this part, figure it out
    for (int i = 0; i < numFramesAvailable; i ++)
    {
        mainFile.write((const char *) pData + (i * nBlockAlign), nBlockAlign);
    }

    //test
    test ++;
    if (test >= nBlockAlign * 120)
    { bComplete = true; }

    //check if our main function is done to finish capture
    if (bComplete)
    { *bDone = true; }

    std::cout << __FUNCTION__ << " numFramesAvailable: " << numFramesAvailable << std::endl;

    return S_OK;
}

HRESULT MyAudioSink::_File_WrapUp()
{
    // (We'll need the final file size to fix the chunk sizes above)
    file_length = mainFile.tellp();

    // Fix the data chunk header to contain the data size
    mainFile.seekp(data_chunk_pos + 4);
    write_word(mainFile, file_length - data_chunk_pos + 8);

    // Fix the file header to contain the proper RIFF chunk size, which is (file size - 8) bytes
    mainFile.seekp(0 + 4);
    write_word(mainFile, file_length - 8, 4);

    mainFile.close();

    std::cout << "finalized file" << std::endl;

    return S_OK;
}
