#include "audio/Utils.h"
#include <iostream>

#define CAPTURE_DEVICE_PROPERTIES_FORMAT  R"(SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\%s\%s\Properties\)"
#define EXCLUSIVE_MODE_PROPERTY           R"({b3f8fa53-0004-438e-9003-51a46e139bfc},3)"
#define EXCLUSIVE_MODE_PRIORITY_PROPERTY  R"({b3f8fa53-0004-438e-9003-51a46e139bfc},4)"
#define MM_RENDER_DEVICE_IDENTIFIER       R"({0.0.0.00000000})"
#define MM_CAPTURE_DEVICE_IDENTIFIER      R"({0.0.1.00000000})"

bool constructMMDeviceRegPropertyPath(const std::string& endpointId, std::string &path)
{
    std::string deviceEndpointId;
    const auto pos = endpointId.find_last_of('.');
    if (pos != std::wstring::npos)
        deviceEndpointId = endpointId.substr(pos + 1);
    else
    {
        std::cout << __FUNCTION__ << " can't parse endpoint id: " << endpointId << " device id: ";
        return false;
    }

    const char* captureOrRenderDevice;
    if (endpointId.find(MM_RENDER_DEVICE_IDENTIFIER) != std::string::npos)
        captureOrRenderDevice = "Render";
    else if (endpointId.find(MM_CAPTURE_DEVICE_IDENTIFIER) != std::string::npos)
        captureOrRenderDevice = "Capture";
    else
    {
        std::cout << __FUNCTION__ << " non mmdevice endpoint: " << endpointId << ", skipped";
        return false;
    }

    char buffer[200]{0};
    snprintf(buffer, 200, CAPTURE_DEVICE_PROPERTIES_FORMAT, captureOrRenderDevice, deviceEndpointId.data());
    path = std::string(buffer);
    return true;
}

int main()
{
    (void)CoInitialize(nullptr);

    std::string deviceRegPropertyPath;
    {
        auto endpointId = getDefaultDeviceEndpointId();
        std::cout << endpointId << std::endl;

        constructMMDeviceRegPropertyPath(endpointId, deviceRegPropertyPath);
        std::cout << deviceRegPropertyPath << std::endl;

        CoUninitialize();
    }

    changeMMDeviceExclusiveModeRegistry(deviceRegPropertyPath, EXCLUSIVE_MODE_PRIORITY_PROPERTY, 1);
    return 0;
}
