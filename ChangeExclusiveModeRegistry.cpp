#include "audio/Utils.h"

int main()
{
    HRESULT hr = CoInitialize(nullptr);

    MyAudioSource pMySource;

    hr = PlayExclusiveStream(&pMySource);

    CoUninitialize();
    return 0;
}
