#include<cstdio>
#include<iostream>
#include<vector>

#include<Windows.h>
#include <devguid.h>
#include <mmdeviceapi.h>
#include <initguid.h>
#include <devpkey.h>
#include<SetupAPI.h>

using namespace std;
#pragma comment(lib, "SetupAPI.lib")

const DWORD properties[30] = { SPDRP_ADDRESS,SPDRP_BUSNUMBER,SPDRP_BUSTYPEGUID,SPDRP_CAPABILITIES,SPDRP_CHARACTERISTICS,
                         SPDRP_CLASS,SPDRP_CLASSGUID,SPDRP_COMPATIBLEIDS,SPDRP_CONFIGFLAGS,SPDRP_DEVICEDESC,SPDRP_DEVTYPE,
                         SPDRP_DRIVER,SPDRP_ENUMERATOR_NAME,SPDRP_EXCLUSIVE,SPDRP_FRIENDLYNAME,SPDRP_HARDWAREID,
                         SPDRP_LEGACYBUSTYPE,SPDRP_LOCATION_INFORMATION,SPDRP_LOWERFILTERS,SPDRP_MFG,SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
                         SPDRP_SECURITY,SPDRP_UI_NUMBER,SPDRP_UI_NUMBER_DESC_FORMAT,SPDRP_UPPERFILTERS };

const char propertiesName[30][35] = { "SPDRP_ADDRESS","SPDRP_BUSNUMBER","SPDRP_BUSTYPEGUID","SPDRP_CAPABILITIES",
                                "SPDRP_CHARACTERISTICS","SPDRP_CLASS","SPDRP_CLASSGUID","SPDRP_COMPATIBLEIDS",
                                "SPDRP_CONFIGFLAGS","SPDRP_DEVICEDESC","SPDRP_DEVTYPE","SPDRP_DRIVER","SPDRP_ENUMERATOR_NAME",
                                "SPDRP_EXCLUSIVE","SPDRP_FRIENDLYNAME","SPDRP_HARDWAREID","SPDRP_LEGACYBUSTYPE",
                                "SPDRP_LOCATION_INFORMATION","SPDRP_LOWERFILTERS","SPDRP_MFG","SPDRP_PHYSICAL_DEVICE_OBJECT_NAME",
                                "SPDRP_SECURITY","SPDRP_UI_NUMBER","SPDRP_UI_NUMBER_DESC_FORMAT","SPDRP_UPPERFILTERS" };

std::wstring GetDeviceProperty(const HDEVINFO& device_info, const PSP_DEVINFO_DATA& device_data, const DEVPROPKEY* requested_property)
{
    DWORD required_size = 0;
    DEVPROPTYPE device_property_type;

    SetupDiGetDevicePropertyW(device_info, device_data, requested_property, &device_property_type, nullptr, 0, &required_size, 0);

    std::vector<BYTE> unicode_buffer(required_size, 0);

    BOOL result = SetupDiGetDevicePropertyW(device_info, device_data, requested_property, &device_property_type, unicode_buffer.data(), required_size, nullptr, 0);
    if (!result)
        return {};

    return { (PWCHAR)unicode_buffer.data() };
}

int main()
{
    FILE *fp = nullptr;
    freopen_s( &fp,"device_information.txt","w",stderr);
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;

    hDevInfo = SetupDiGetClassDevs(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        fprintf( fp, "fail to get information of devices..");
        fprintf( fp, "This will go to the file 'device_information.txt'\n");
        fclose(fp);
        return 0;
    }

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (DWORD i = 0; ; ++i)
    {
        if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData))
            break;

        wchar_t propertyBuf[MAX_PATH] = { 0 };
        DWORD bufSize;
        int j = 0;

        fprintf(fp, "Device\tNo.%ld\n", i);
        DEVPROPKEY busRelKey = DEVPKEY_Device_BusRelations;

        auto busRel = GetDeviceProperty(hDevInfo, &DeviceInfoData, &busRelKey);
        if (!busRel.empty())
        {
            fprintf(fp, "DEVPKEY_Device_BusRelations:\t%S\n", busRel.data());
        }

        for (; j < 25; j++) {

            DWORD current_property = properties[j];
            if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, current_property, nullptr, (PBYTE)propertyBuf, MAX_PATH, &bufSize))
                continue;

            fprintf(fp, "%s:\t%S\n", propertiesName[j], propertyBuf);
        }
        fprintf(fp, "\n\n");
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    fclose(fp);

    std::wcout << "Check device_information.txt" << std::endl;
    return 0;
}
