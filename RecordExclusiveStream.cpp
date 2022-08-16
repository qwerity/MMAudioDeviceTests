#include "audio/Utils.h"

int main()
{
    HRESULT hr = CoInitialize(nullptr);

    MyAudioSink pMySink;
    hr = RecordExclusiveStream(&pMySink);

    CoUninitialize();
    return 0;
}
