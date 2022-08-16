#include "Utils.h"
#include "MyAudioSource.h"
#include "MyAudioSink.h"

// Windows multimedia device
#include <Mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

#include "avrt.h"
#include "uuids.h"

#include <vector>
#include <iostream>

//https://docs.microsoft.com/en-us/windows/win32/coreaudio/exclusive-mode-streams

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define RETURN_ON_ERROR(hres)  \
              if (FAILED(hres)) { return; }
#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { printf("line: %d, latest HR:%ld\n", __LINE__, hres); goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != nullptr)  \
                { (punk)->Release(); (punk) = nullptr; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

#include <comdef.h>

#define CHECKHR(hr) if(FAILED(hr)) throw _com_error(hr);
#define SAFE_RELEASE(punk)  \
              if ((punk) != nullptr)  \
                { (punk)->Release(); (punk) = nullptr; }

HRESULT RecordExclusiveStream(MyAudioSink *pMySink)
{
    HRESULT hr = S_OK;
    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    REFERENCE_TIME hnsActualDuration;
    UINT32 bufferFrameCount;
    UINT32 numFramesAvailable;
    IMMDeviceEnumerator *pEnumerator = nullptr;
    IMMDevice *pDevice = nullptr;
    IAudioClient *pAudioClient = nullptr;
    IAudioCaptureClient *pCaptureClient = nullptr;
    WAVEFORMATEX *pwfx = nullptr;
    UINT32 packetLength = 0;
    BOOL bDone = FALSE;
    BYTE *pData;
    DWORD flags;

    try {
        CHECKHR(CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator));
        CHECKHR(pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice));
        CHECKHR(pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void**)&pAudioClient));

        // Call a helper function to negotiate with the audio
        // device for an exclusive-mode stream format.
        WAVEFORMATEXTENSIBLE _format;
        CHECKHR(GetStreamFormat(AUDCLNT_SHAREMODE_EXCLUSIVE, pAudioClient, _format));
        pwfx = &_format.Format;

        CHECKHR(pAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, 0, hnsRequestedDuration, 0, pwfx, nullptr));

        // Get the size of the allocated buffer.
        CHECKHR(pAudioClient->GetBufferSize(&bufferFrameCount));
        CHECKHR(pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient));

        // Notify the audio sink which format to use.
        CHECKHR(pMySink->SetFormat(pwfx));

        // Calculate the actual duration of the allocated buffer.
        hnsActualDuration = (double)REFTIMES_PER_SEC * bufferFrameCount / pwfx->nSamplesPerSec;

        CHECKHR(pAudioClient->Start());  // Start recording.

        // Each loop fills about half of the shared buffer.
        while (bDone == FALSE)
        {
            // Sleep for half the buffer duration.
            Sleep(hnsActualDuration/REFTIMES_PER_MILLISEC/2);

            CHECKHR(pCaptureClient->GetNextPacketSize(&packetLength));

            while (packetLength != 0)
            {
                // Get the available data in the shared buffer.
                CHECKHR(pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr));

                if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
                    pData = nullptr;  // Tell CopyData to write silence.

                // Copy the available capture data to the audio sink.
                CHECKHR(pMySink->CopyData(pData, numFramesAvailable, &bDone));
                CHECKHR(pCaptureClient->ReleaseBuffer(numFramesAvailable));
                CHECKHR(pCaptureClient->GetNextPacketSize(&packetLength));
            }
        }

        CHECKHR(pAudioClient->Stop());  // Stop recording.
    }
    catch (_com_error& e_com) {
        std::cout << __FUNCTION__ << " failed: " << e_com.ErrorMessage();
    }
    catch (const std::exception& e) {
        std::cout << __FUNCTION__ << " failed: " << e.what();
    }

    CoTaskMemFree(pwfx);
    SAFE_RELEASE(pEnumerator)
    SAFE_RELEASE(pDevice)
    SAFE_RELEASE(pAudioClient)
    SAFE_RELEASE(pCaptureClient)

    return hr;
}

HRESULT GetStreamFormat(AUDCLNT_SHAREMODE mode, IAudioClient* _audioClient, WAVEFORMATEXTENSIBLE& mixFormat) {
    HRESULT hr;
    WAVEFORMATEXTENSIBLE format;
    format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE);
    format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    format.Format.wBitsPerSample = 32;

    std::vector<int> sampleRates{48000, 96000, 44100, 192000};

    // FLOAT
    format.Format.nChannels = 2;
    format.Samples.wValidBitsPerSample = 32;
    format.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    format.SubFormat = MEDIASUBTYPE_IEEE_FLOAT;

    for(auto rate : sampleRates) {
        format.Format.nSamplesPerSec = rate;
        format.Format.nAvgBytesPerSec = format.Format.nSamplesPerSec * format.Format.nChannels * format.Format.wBitsPerSample / 8;
        format.Format.nBlockAlign = (format.Format.wBitsPerSample * format.Format.nChannels / 8);
        WAVEFORMATEXTENSIBLE * match;
        hr = _audioClient->IsFormatSupported(mode, (WAVEFORMATEX*)&format, (WAVEFORMATEX**)&match);
        if (hr == S_OK || hr == S_FALSE) {
            if (!match)
                match = &format;
            if ((*match).Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
                mixFormat = *match;
            else
                mixFormat.Format = match->Format;

            return S_OK;
        }
    }

    // PCM 32
    format.Samples.wValidBitsPerSample = 32;
    format.Format.wBitsPerSample = 32;
    format.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    format.SubFormat = MEDIASUBTYPE_PCM;

    for(auto rate : sampleRates) {
        format.Format.nSamplesPerSec = rate;
        format.Format.nAvgBytesPerSec = format.Format.nSamplesPerSec * format.Format.nChannels * format.Format.wBitsPerSample / 8;
        format.Format.nBlockAlign = (WORD)(format.Format.wBitsPerSample * format.Format.nChannels / 8);
        WAVEFORMATEXTENSIBLE * match;
        hr = _audioClient->IsFormatSupported(mode, (WAVEFORMATEX*)&format, (WAVEFORMATEX**)&match);
        if (hr == S_OK || hr == S_FALSE) {
            if (!match)
                match = &format;
            if ((*match).Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
                mixFormat = *match;
            else
                mixFormat.Format = match->Format;

            return S_OK;
        }
    }

    // PCM 24
    format.Samples.wValidBitsPerSample = 24;
    format.Format.wBitsPerSample = 32;
    format.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    format.SubFormat = MEDIASUBTYPE_PCM;

    for (auto rate : sampleRates) {
        format.Format.nSamplesPerSec = rate;
        format.Format.nAvgBytesPerSec = format.Format.nSamplesPerSec * format.Format.nChannels * format.Format.wBitsPerSample / 8;
        format.Format.nBlockAlign = (WORD)(format.Format.wBitsPerSample * format.Format.nChannels / 8);
        WAVEFORMATEXTENSIBLE * match;
        hr = _audioClient->IsFormatSupported(mode, (WAVEFORMATEX*)&format, (WAVEFORMATEX**)&match);
        if (hr == S_OK || hr == S_FALSE) {
            if (!match)
                match = &format;
            if ((*match).Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
                mixFormat = *match;
            else
                mixFormat.Format = match->Format;

            return S_OK;
        }
    }

    // PCM 16
    format.Samples.wValidBitsPerSample = 16;
    format.Format.wBitsPerSample = 16;
    format.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    format.SubFormat = MEDIASUBTYPE_PCM;
    for(auto rate: sampleRates) {
        format.Format.nSamplesPerSec = rate;
        format.Format.nAvgBytesPerSec = format.Format.nSamplesPerSec * format.Format.nChannels * format.Format.wBitsPerSample / 8;
        format.Format.nBlockAlign = (WORD)(format.Format.wBitsPerSample * format.Format.nChannels / 8);
        WAVEFORMATEXTENSIBLE * match;
        hr = _audioClient->IsFormatSupported(mode, (WAVEFORMATEX*)&format, (WAVEFORMATEX**)&match);
        if (hr == S_OK || hr == S_FALSE) {
            if (!match)
                match = &format;
            if ((*match).Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
                mixFormat = *match;
            else
                mixFormat.Format = match->Format;

            return S_OK;
        }
    }

    format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE);
    format.Format.wFormatTag = WAVE_FORMAT_PCM;
    format.Format.wBitsPerSample = 16;
    for(auto rate: sampleRates) {
        format.Format.nSamplesPerSec = rate;
        format.Format.nAvgBytesPerSec = format.Format.nSamplesPerSec * format.Format.nChannels * format.Format.wBitsPerSample / 8;
        format.Format.nBlockAlign = (WORD)(format.Format.wBitsPerSample * format.Format.nChannels / 8);
        format.SubFormat = MEDIASUBTYPE_IEEE_FLOAT;
        WAVEFORMATEXTENSIBLE * match;
        hr = _audioClient->IsFormatSupported(mode, (WAVEFORMATEX*)&format, (WAVEFORMATEX**)&match);
        if (hr == S_OK || hr == S_FALSE) {
            if (!match)
                match = &format;
            if ((*match).Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
                mixFormat = *match;
            else
                mixFormat.Format = match->Format;

            return S_OK;
        }
    }
    return E_FAIL;
}

void printDeviceInfo(IMMDevice *pDevice) {
    HRESULT hr = S_OK;
    LPWSTR pwszID = nullptr;
    IPropertyStore *pProps = nullptr;

    // Get the endpoint ID string.
    hr = pDevice->GetId(&pwszID);
    RETURN_ON_ERROR(hr)

    hr = pDevice->OpenPropertyStore(STGM_READWRITE, &pProps);
    RETURN_ON_ERROR(hr)

    PROPVARIANT varName;
    // Initialize container for property value.
    PropVariantInit(&varName);

    // Get the endpoint's friendly-name property.
    hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
    RETURN_ON_ERROR(hr)

    // Print endpoint friendly name and endpoint ID.
    printf("Device: \"%S\" (%S)\n", varName.pwszVal, pwszID);

    CoTaskMemFree(pwszID);
}

std::string getDefaultDeviceEndpointId()
{
    HRESULT hr = S_OK;
    IMMDeviceEnumerator *pEnumerator = nullptr;
    IMMDevice *pDevice = nullptr;
    LPWSTR pwszID = nullptr;

    try
    {
        CHECKHR(CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void **) &pEnumerator));
        CHECKHR(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice));

        // Get the endpoint ID string.
        CHECKHR(pDevice->GetId(&pwszID));
    }
    catch (_com_error& e_com) {
        std::cout << __FUNCTION__ << " failed: " << e_com.ErrorMessage();
    }
    catch (const std::exception& e) {
        std::cout << __FUNCTION__ << " failed: " << e.what();
    }

    std::wstring endpointId(pwszID);

    CoTaskMemFree(pwszID);
    SAFE_RELEASE(pDevice)
    SAFE_RELEASE(pEnumerator)

    return std::string(endpointId.begin(), endpointId.end());
}

HRESULT PlayExclusiveStream(MyAudioSource *pMySource)
{
    HRESULT hr;
    REFERENCE_TIME hnsRequestedDuration = 0;
    IMMDeviceEnumerator *pEnumerator = nullptr;
    IMMDevice *pDevice = nullptr;
    IAudioClient *pAudioClient = nullptr;
    IAudioRenderClient *pRenderClient = nullptr;
    WAVEFORMATEX *pwfx = nullptr;
    HANDLE hEvent = nullptr;
    HANDLE hTask = nullptr;
    UINT32 bufferFrameCount;
    BYTE *pData;
    DWORD flags = 0;
    DWORD taskIndex = 0;

    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
    EXIT_ON_ERROR(hr)

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    EXIT_ON_ERROR(hr)

    printDeviceInfo(pDevice);

    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void**)&pAudioClient);
    EXIT_ON_ERROR(hr)

    // Call a helper function to negotiate with the audio
    // device for an exclusive-mode stream format.
    WAVEFORMATEXTENSIBLE _format;
    hr = GetStreamFormat(AUDCLNT_SHAREMODE_EXCLUSIVE, pAudioClient, _format);
    EXIT_ON_ERROR(hr)

    pwfx = &_format.Format;

    // Initialize the stream to play at the minimum latency.
    hr = pAudioClient->GetDevicePeriod(nullptr, &hnsRequestedDuration);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_EXCLUSIVE,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        hnsRequestedDuration,
        hnsRequestedDuration,
        pwfx,
        nullptr);
    if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
        // Align the buffer if needed, see IAudioClient::Initialize() documentation
        UINT32 nFrames = 0;
        hr = pAudioClient->GetBufferSize(&nFrames);
        EXIT_ON_ERROR(hr)
        hnsRequestedDuration = (REFERENCE_TIME)((double)REFTIMES_PER_SEC / pwfx->nSamplesPerSec * nFrames + 0.5);
        hr = pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_EXCLUSIVE,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            hnsRequestedDuration,
            hnsRequestedDuration,
            pwfx,
            nullptr);
    }
    EXIT_ON_ERROR(hr)

    // Tell the audio source which format to use.
    hr = pMySource->SetFormat(pwfx);
    EXIT_ON_ERROR(hr)

    // Create an event handle and register it for
    // buffer-event notifications.
    hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (hEvent == nullptr)
    {
        hr = E_FAIL;
        goto Exit;
    }

    hr = pAudioClient->SetEventHandle(hEvent);
    EXIT_ON_ERROR(hr)

    // Get the actual size of the two allocated buffers.
    hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->GetService(IID_IAudioRenderClient, (void**)&pRenderClient);
    EXIT_ON_ERROR(hr)

    // To reduce latency, load the first buffer with data
    // from the audio source before starting the stream.
    hr = pRenderClient->GetBuffer(bufferFrameCount, &pData);
    EXIT_ON_ERROR(hr)

    hr = pMySource->LoadData(bufferFrameCount, pData, &flags);
    EXIT_ON_ERROR(hr)

    hr = pRenderClient->ReleaseBuffer(bufferFrameCount, flags);
    EXIT_ON_ERROR(hr)

    // Ask MMCSS to temporarily boost the thread priority
    // to reduce glitches while the low-latency stream plays.
    hTask = AvSetMmThreadCharacteristics(TEXT("Pro Audio"), &taskIndex);
    if (hTask == nullptr)
    {
        hr = E_FAIL;
        EXIT_ON_ERROR(hr)
    }

    hr = pAudioClient->Start();  // Start playing.
    EXIT_ON_ERROR(hr)

    // Each loop fills one of the two buffers.
    while (flags != AUDCLNT_BUFFERFLAGS_SILENT)
    {
        // Wait for next buffer event to be signaled.
        DWORD retval = WaitForSingleObject(hEvent, 2000);
        if (retval != WAIT_OBJECT_0)
        {
            // Event handle timed out after a 2-second wait.
            pAudioClient->Stop();
            hr = ERROR_TIMEOUT;
            goto Exit;
        }

        // Grab the next empty buffer from the audio device.
        hr = pRenderClient->GetBuffer(bufferFrameCount, &pData);
        EXIT_ON_ERROR(hr)

        // Load the buffer with data from the audio source.
        hr = pMySource->LoadData(bufferFrameCount, pData, &flags);
        EXIT_ON_ERROR(hr)

        hr = pRenderClient->ReleaseBuffer(bufferFrameCount, flags);
        EXIT_ON_ERROR(hr)
    }

    // Wait for the last buffer to play before stopping.
    Sleep((DWORD)(hnsRequestedDuration/REFTIMES_PER_MILLISEC));

    hr = pAudioClient->Stop();  // Stop playing.
    EXIT_ON_ERROR(hr)

    Exit:
    if (hEvent != nullptr)
    {
        CloseHandle(hEvent);
    }
    if (hTask != nullptr)
    {
        AvRevertMmThreadCharacteristics(hTask);
    }
    CoTaskMemFree(pwfx);
    SAFE_RELEASE(pEnumerator)
    SAFE_RELEASE(pDevice)
    SAFE_RELEASE(pAudioClient)
    SAFE_RELEASE(pRenderClient)

    return hr;
}

bool changeMMDeviceExclusiveModeRegistry(const std::string& deviceRegPropertyPath, const char* property, DWORD newValue)
{
    HKEY hRegKey;

    auto lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, deviceRegPropertyPath.data(), 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WOW64_64KEY | KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hRegKey, nullptr);
    if (lResult != ERROR_SUCCESS)
    {
        printf("Error create/opening key: %ld\n", lResult);
        return FALSE;
    }

    DWORD dwProperty;
    DWORD cbProperty = sizeof(dwProperty);
    DWORD dwType;

    lResult = RegQueryValueEx(hRegKey, property, nullptr, &dwType, (BYTE *) &dwProperty, &cbProperty);
    if (ERROR_SUCCESS != lResult)
    {
        if (lResult == ERROR_FILE_NOT_FOUND)
        {
            printf("Key not found2.\n");
            return TRUE;
        }
        else
        {
            printf("Error opening key: %ld\n", lResult);
            return FALSE;
        }
    }
    std::cout << "dwProperty: " << dwProperty << std::endl;

    lResult = RegSetValueEx(hRegKey, property, 0, REG_DWORD, (BYTE *) &newValue, sizeof(DWORD));
    if (ERROR_SUCCESS != lResult)
    {
        if (lResult == ERROR_FILE_NOT_FOUND)
        {
            printf("Key not found2.\n");
            return TRUE;
        }
        else
        {
            printf("Error opening key: %ld\n", lResult);
            return FALSE;
        }
    }

    lResult = RegQueryValueEx(hRegKey, property, nullptr, &dwType, (BYTE *) &dwProperty, &cbProperty);
    if (ERROR_SUCCESS != lResult)
    {
        if (lResult == ERROR_FILE_NOT_FOUND)
        {
            printf("Key not found2.\n");
            return TRUE;
        }
        else
        {
            printf("Error opening key: %ld\n", lResult);
            return FALSE;
        }
    }

    RegCloseKey(hRegKey);
    std::cout << "dwProperty: " << dwProperty << std::endl;
    return true;
}
