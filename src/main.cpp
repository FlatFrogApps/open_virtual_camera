#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfobjects.h>
#include <mftransform.h>
#include <Mferror.h>
#include <iostream>
#include <vector>
#include <Windows.h>
#include <atlbase.h>
#include <fstream>
#include <set>
#include <map>
#include "virtual_output.h"

#pragma comment(lib, "mf")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfuuid")

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

class MediaFoundationInitializer {
public:
    MediaFoundationInitializer() {
        MFStartup(MF_VERSION);
    }
    ~MediaFoundationInitializer() {
        MFShutdown();
    }
};


void ErrorDescription(HRESULT hr) 
{ 
     if(FACILITY_WINDOWS == HRESULT_FACILITY(hr)) 
         hr = HRESULT_CODE(hr); 
     TCHAR* szErrMsg; 

     if(FormatMessage( 
       FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, 
       NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
       (LPTSTR)&szErrMsg, 0, NULL) != 0) 
     { 
         _tprintf(TEXT("%s"), szErrMsg); 
         LocalFree(szErrMsg); 
     } else 
         _tprintf( TEXT("[Could not find a description for error # %#x.]\n"), hr); 
}


bool SetVideoCaptureResolution(IMFSourceReader* pReader, UINT32* width, UINT32* height, UINT32* fps, bool four_k) {
    std::map<UINT32, UINT32> possible_res = {
        {1920, 1080},
        {1280, 720}
    };
    if (four_k) {
        possible_res.emplace(3840, 2160);
    }
    UINT32 highest_res = 0;
    DWORD highest_res_i = -1;
    CComPtr<IMFMediaType> pType;
    DWORD i = 0;

    while (SUCCEEDED(pReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, &pType))) {
        GUID subtype = { 0 };
        pType->GetGUID(MF_MT_SUBTYPE, &subtype);

        if (subtype == MFVideoFormat_NV12) {
            UINT32 currentWidth = 0, currentHeight = 0, res = 0, pNumerator = 0, pDenominator = 0;
            MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &currentWidth, &currentHeight);
            MFGetAttributeRatio(pType, MF_MT_FRAME_RATE, &pNumerator, &pDenominator);
            res = currentWidth * currentHeight * pNumerator / pDenominator;
            if (res > highest_res && possible_res[currentWidth] == currentHeight) {
                highest_res = res;
                highest_res_i = i;
                *width = currentWidth;
                *height = currentHeight;
                *fps = pNumerator / pDenominator;
            }
        }

        pType.Release();
        i++;
    }

    if (highest_res_i >= 0 && SUCCEEDED(pReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, highest_res_i, &pType))) {
        HRESULT hr = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pType);
        if (SUCCEEDED(hr)) {
            std::cout << "Successfully set the resolution to " << *width << "x" << *height << " with " << *fps << " fps" << std::endl;
            return true;
        } else {
            std::cerr << "Failed to set media type." << std::endl;
            ErrorDescription(hr);
        }
    }

    std::cerr << "Desired resolution not found." << std::endl;
    return false;
}

void CameraLoop(const wchar_t *camera_name, 
                CComPtr<IMFAttributes> &pAttributes,
                CComPtr<IMFMediaSource> &pSource,
                CComPtr<IMFSourceReader> &pReader,
                IMFActivate** &ppDevices,
                bool four_k)
{

    HRESULT hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) {
        std::cerr << "SetGUID failed." << std::endl;
        ErrorDescription(hr);
        return;
    }

    UINT32 count = 0;

    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr)) {
        std::cerr << "MFEnumDeviceSources failed." << std::endl;
        ErrorDescription(hr);
        return;
    }

    if (count == 0) {
        std::cerr << "No video capture devices found." << std::endl;
        return;
    }

    int device_index = -1;
    for (DWORD i = 0; i < count; i++)
    {
        WCHAR* szFriendlyName = NULL;
        hr = ppDevices[i]->GetAllocatedString(
            MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
            &szFriendlyName,
            NULL
        );

        if (FAILED(hr))
        {
            ErrorDescription(hr);
            return;
        }

        if (0 == wcscmp(szFriendlyName, camera_name))
        {
            device_index = i;
        }
        CoTaskMemFree(szFriendlyName);
    }

    if (device_index >= 0) {
        std::wcout << "Camera " << camera_name << " identified as device " << device_index << std::endl;
    }
    else {
        device_index = 0;
        std::wcout << "Camera with name " << camera_name << " specified in file camera_name.txt not found, using device " << device_index << std::endl;
    }

    hr = ppDevices[device_index]->ActivateObject(IID_PPV_ARGS(&pSource));
    if (FAILED(hr)) {
        std::cerr << "ActivateObject failed." << std::endl;
        ErrorDescription(hr);
        return;
    }

    for (UINT32 i = 0; i < count; i++) {
        SafeRelease(&ppDevices[i]);
    }
    CoTaskMemFree(ppDevices);

    hr = MFCreateSourceReaderFromMediaSource(pSource, pAttributes, &pReader);
    if (FAILED(hr)) {
        std::cerr << "MFCreateSourceReaderFromMediaSource failed." << std::endl;
        ErrorDescription(hr);
        return;
    }

    UINT32 width;
    UINT32 height;
    UINT32 fps;

    if (!SetVideoCaptureResolution(pReader, &width, &height, &fps, four_k)){
        return;
    }

    VirtualOutput virtual_output(width, height, fps);

    while (true) {
        DWORD streamIndex, flags;
        LONGLONG llTimeStamp;
        CComPtr<IMFSample> pSample;
        hr = pReader->ReadSample(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            &streamIndex,
            &flags,
            &llTimeStamp,
            &pSample
        );

        if (FAILED(hr)) {
            std::cerr << "ReadSample failed." << std::endl;
            ErrorDescription(hr);
            break;
        }

        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
            std::cout << "End of stream." << std::endl;
            break;
        }

        if (pSample) {
            CComPtr<IMFMediaBuffer> pBuffer;
            hr = pSample->ConvertToContiguousBuffer(&pBuffer);
            if (FAILED(hr)) {
                std::cerr << "ConvertToContiguousBuffer failed." << std::endl;
                ErrorDescription(hr);
                break;
            }

            BYTE* pData = nullptr;
            DWORD cbBuffer = 0;

            hr = pBuffer->Lock(&pData, nullptr, &cbBuffer);
            if (FAILED(hr)) {
                std::cerr << "Lock failed." << std::endl;
                ErrorDescription(hr);
                break;
            }

            // Process the raw byte stream (pData, cbBuffer)
            if (cbBuffer > 0) virtual_output.send(static_cast<uint8_t*>(pData));

            hr = pBuffer->Unlock();
            if (FAILED(hr)) {
                std::cerr << "Unlock failed." << std::endl;
                ErrorDescription(hr);
                break;
            }
        }
    }
    virtual_output.stop();
}

void ReadCameraStream(const wchar_t *camera_name, bool four_k)
{
    while (true)
    {
        MediaFoundationInitializer mfInitializer;

        CComPtr<IMFAttributes> pAttributes;
        CComPtr<IMFMediaSource> pSource;
        CComPtr<IMFSourceReader> pReader;
        IMFActivate** ppDevices = nullptr;
        HRESULT hr = MFCreateAttributes(&pAttributes, 1);

        if (SUCCEEDED(hr)) {
            try {
                CameraLoop(camera_name, pAttributes, pSource, pReader, ppDevices, four_k);
            } catch (std::runtime_error err) {
                std::cout << err.what() << std::endl;
            }
        } else {
            std::cerr << "MFCreateAttributes failed." << std::endl;
            ErrorDescription(hr);
        }
        UINT32 count = 0;
        hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
        if (FAILED(hr)) {
            std::cerr << "MFEnumDeviceSources failed." << std::endl;
            ErrorDescription(hr);
        }

        SafeRelease(&pAttributes);

        for (DWORD i = 0; i < count; i++)
        {
            SafeRelease(&ppDevices[i]);
        }
        CoTaskMemFree(ppDevices);
        SafeRelease(&pSource);
        std::cout << "Trying again..." << std::endl;
        Sleep(5000);
    }
}

int main(int argc, char** argv) 
{
    bool four_k = true;
    if (argc < 2 || argv[argc - 1][0] != 'd') {
        FreeConsole();
    }
    if (argc > 1 && argv[1][0] == '1') {
        four_k = false;
    }

    wchar_t buffer[MAX_PATH], drive[MAX_PATH] ,directory[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH); 
    _wsplitpath(buffer, drive, directory, NULL, NULL);
    std::wstring s_drive(drive), s_dir(directory);

    std::wifstream camera_name_file(s_drive + s_dir + L"camera_name.txt");
    wchar_t camera_name[256];
    if(camera_name_file.good()){
        camera_name_file.getline(camera_name, 256);
    } else {
        std::cout << "camera_name.txt file not found" << std::endl;
    }
    camera_name_file.close();

    ReadCameraStream(camera_name, four_k);
    return 0;
}
