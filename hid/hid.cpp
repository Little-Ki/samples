#include "hid.h"

#include <vector>

const static GUID HID_GUID = {0x4D1E55B2, 0xF16F, 0x11CF, {0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30}};

BYTE HID::report_id(const Device &device, HANDLE device_handle, HIDP_REPORT_TYPE type) {
    UINT16 CapsSize;
    PHIDP_PREPARSED_DATA DataPtr;
    BYTE Out;

    if (device.caps.NumberOutputButtonCaps != 0) {
        CapsSize = (type == HIDP_REPORT_TYPE::HidP_Feature) ? device.caps.NumberFeatureButtonCaps : device.caps.NumberOutputButtonCaps;
        auto ButtonCaps = std::make_unique<HIDP_BUTTON_CAPS>(CapsSize);
        HidD_GetPreparsedData(device_handle, &DataPtr);
        HidP_GetButtonCaps(type, ButtonCaps.get(), &CapsSize, DataPtr);
        Out = ButtonCaps.get()[0].ReportID;
    } else {
        CapsSize = (type == HIDP_REPORT_TYPE::HidP_Feature) ? device.caps.NumberFeatureValueCaps : device.caps.NumberOutputValueCaps;
        auto ValueCaps = std::make_unique<HIDP_VALUE_CAPS>(CapsSize);
        HidD_GetPreparsedData(device_handle, &DataPtr);
        HidP_GetValueCaps(type, ValueCaps.get(), &CapsSize, DataPtr);
        Out = ValueCaps.get()[0].ReportID;
    }

    return Out;
}

string_t HID::device_path(HDEVINFO device_info_handle, SP_DEVICE_INTERFACE_DATA &device_info_data) {
    DWORD Size = 0;

    if (!SetupDiGetDeviceInterfaceDetail(
            device_info_handle,
            &device_info_data,
            NULL,
            0,
            &Size,
            NULL)) {
        SP_DEVICE_INTERFACE_DETAIL_DATA Detail;

        Detail.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (SetupDiGetDeviceInterfaceDetail(
                device_info_handle,
                &device_info_data,
                &Detail,
                Size,
                &Size,
                NULL)) {
            return string_t(Detail.DevicePath);
        }
    }

    return string_t();
}

void HID::enum_device(std::function<bool(const Device &)> callback) {
    uint32_t index = 0;

    HDEVINFO dev_handle = SetupDiGetClassDevs(
        &HID_GUID,
        NULL,
        0,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    HIDD_ATTRIBUTES attributes;

    SP_DEVICE_INTERFACE_DATA dev_data;
    dev_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    while (SetupDiEnumDeviceInterfaces(dev_handle, NULL, &HID_GUID, index, &dev_data)) {
        auto dev_path = device_path(dev_handle, dev_data);
        uint32_t flag = 0;
        HANDLE dev_handle =
            CreateFile(
                dev_path.c_str(),
                GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                flag,
                NULL);
        if (dev_handle) {
            attributes.Size = sizeof(_HIDD_ATTRIBUTES);
            if (HidD_GetAttributes(dev_handle, &attributes)) {
                Device device;
                HIDP_CAPS caps;
                PHIDP_PREPARSED_DATA data_ptr;
                HidD_GetPreparsedData(dev_handle, &data_ptr);
                HidP_GetCaps(data_ptr, &caps);
                HidD_FreePreparsedData(data_ptr);
                device.report_id_future = report_id(device, dev_handle, HIDP_REPORT_TYPE::HidP_Feature);
                device.report_id_output = report_id(device, dev_handle, HIDP_REPORT_TYPE::HidP_Output);
                device.device_handle = dev_handle;
                if (callback(device)) {
                    SetupDiDestroyDeviceInfoList(dev_handle);
                    return;
                }
                CloseHandle(dev_handle);
            }
        }
        index++;
    };
    SetupDiDestroyDeviceInfoList(dev_handle);
}

bool HID::write(const Device &device, BYTE *data, int length) {
    if (device.caps.OutputReportByteLength == 0 ||
        length != device.caps.OutputReportByteLength - 1) {
        return false;
    } else {
        DWORD count;

        auto buffer = std::vector<BYTE>(device.caps.OutputReportByteLength);

        buffer[0] = device.report_id_output;

        std::copy(data, data + length, &buffer[1]);

        bool Out = WriteFile(
            device.device_handle,
            buffer.data(),
            device.caps.OutputReportByteLength,
            &count,
            0);

        return Out;
    }
}

bool HID::read(const Device &device, BYTE *data, int length) {
    if (device.caps.InputReportByteLength == 0 ||
        length != device.caps.InputReportByteLength) {
        return false;
    } else {
        HANDLE event_handle = CreateEvent(NULL, true, false, NULL);

        OVERLAPPED overlapped = {0, 0, 0, 0, event_handle};

        bool Out = ReadFile(
            device.device_handle,
            data,
            length,
            0,
            &overlapped);

        WaitForSingleObject(event_handle, 500);

        return Out;
    }
}
