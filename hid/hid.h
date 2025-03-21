#pragma once
#define WIN32_LEAN_AND_MEAN // �� Windows ͷ�ļ����ų�����ʹ�õ�����

#include <Windows.h>

#include <SetupAPI.h>
#include <hidpi.h>
#include <hidsdi.h>
#include <hidusage.h>

#include <iomanip>
#include <iostream>
#include <vector>

#include <functional>
#include <map>
#include <memory>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

extern "C" void __stdcall HidD_GetHidGuid(OUT LPGUID hidGuid);
extern "C" BOOLEAN __stdcall HidD_GetAttributes(IN HANDLE device, OUT HIDD_ATTRIBUTES *attributes);
extern "C" BOOLEAN __stdcall HidD_GetManufacturerString(IN HANDLE device, OUT void *buffer, IN ULONG bufferLen);
extern "C" BOOLEAN __stdcall HidD_GetProductString(IN HANDLE device, OUT void *buffer, IN ULONG bufferLen);
extern "C" BOOLEAN __stdcall HidD_GetSerialNumberString(IN HANDLE device, OUT void *buffer, IN ULONG bufferLen);
extern "C" BOOLEAN __stdcall HidD_GetFeature(IN HANDLE device, OUT void *reportBuffer, IN ULONG bufferLen);
extern "C" BOOLEAN __stdcall HidD_SetFeature(IN HANDLE device, IN void *reportBuffer, IN ULONG bufferLen);
extern "C" BOOLEAN __stdcall HidD_GetNumInputBuffers(IN HANDLE device, OUT ULONG *numBuffers);
extern "C" BOOLEAN __stdcall HidD_SetNumInputBuffers(IN HANDLE device, OUT ULONG numBuffers);

struct Device {
    HIDD_ATTRIBUTES attributes;
    HIDP_CAPS caps;
    BYTE report_id_future;
    BYTE report_id_output;
    HANDLE device_handle;
};

#ifdef UNICODE
    using string_t = std::wstring;
#else
    using string_t = std::string;
#endif

class HID {
private:

    BYTE report_id(const Device &, HANDLE, HIDP_REPORT_TYPE);

    string_t device_path(HDEVINFO, SP_DEVICE_INTERFACE_DATA &);

public:
    void enum_device(std::function<bool(const Device &)> callback);

    bool write(const Device &device, BYTE *data, int length);

    bool read(const Device &device, BYTE *data, int length);
};