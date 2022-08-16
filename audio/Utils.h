#pragma once

#include <initguid.h>

#include "MyAudioSink.h"
#include "MyAudioSource.h"

std::string getDefaultDeviceEndpointId();
HRESULT GetStreamFormat(AUDCLNT_SHAREMODE mode, IAudioClient* _audioClient, WAVEFORMATEXTENSIBLE& mixFormat);

HRESULT RecordExclusiveStream(MyAudioSink *pMySink);
HRESULT PlayExclusiveStream(MyAudioSource *pMySource);

bool changeMMDeviceExclusiveModeRegistry(const std::string& deviceRegPropertyPath, const char* property, DWORD newValue);

