/*
 * Modified example code from https://learn.microsoft.com/en-us/windows/win32/medfound/enumerating-video-capture-devices
*/

#include <mfidl.h>
#include <mfapi.h>

#pragma comment(lib, "mf")
#pragma comment(lib, "mfplat")

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

DWORD FindDeviceByName(const wchar_t *camera_name)
{
    WCHAR** ppszName;
    DWORD device_index = -1;
    IMFMediaSource *pSource = NULL;
    IMFAttributes *pAttributes = NULL;
    IMFActivate **ppDevices = NULL;

    // Create an attribute store to specify the enumeration parameters.
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr))
    {
        goto done;
    }

    // Source type: video capture devices
    hr = pAttributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, 
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
        );
    if (FAILED(hr))
    {
        goto done;
    }

    // Enumerate devices.
    UINT32 count;
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr))
    {
        goto done;
    }

    if (count == 0)
    {
        hr = E_FAIL;
        goto done;
    }
    for (DWORD i = 0; i < count; i++)
    {
        hr = ppDevices[i]->GetAllocatedString(
                MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
                ppszName,
                NULL
            );
        if (FAILED(hr))
        {
            goto done;
        }
        if (0 == wcscmp(*ppszName, camera_name))
        {
            device_index = i;
        }
    }
done:
    SafeRelease(&pAttributes);

    for (DWORD i = 0; i < count; i++)
    {
        SafeRelease(&ppDevices[i]);
    }
    CoTaskMemFree(ppDevices);
    SafeRelease(&pSource);
    return device_index;
}
