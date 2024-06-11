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
#include "virtual_output.h"

#pragma comment(lib, "mf")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfreadwrite")


class MediaFoundationInitializer {
public:
    MediaFoundationInitializer() {
        MFStartup(MF_VERSION);
    }
    ~MediaFoundationInitializer() {
        MFShutdown();
    }
};

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

void ReadCameraStream(const wchar_t *camera_name)
{
    MediaFoundationInitializer mfInitializer;

    CComPtr<IMFAttributes> pAttributes;
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) {
        std::cerr << "MFCreateAttributes failed." << std::endl;
        return;
    }

    hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) {
        std::cerr << "SetGUID failed." << std::endl;
        return;
    }

    UINT32 count = 0;
    IMFActivate** ppDevices = nullptr;
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr)) {
        std::cerr << "MFEnumDeviceSources failed." << std::endl;
        return;
    }

    if (count == 0) {
        std::cerr << "No video capture devices found." << std::endl;
        return;
    }

    CComPtr<IMFMediaSource> pSource;
    CComPtr<IMFSourceReader> pReader;
    VirtualOutput *virtual_output;
    int device_index = 0;

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
            goto done;
        }

        if (0 == wcscmp(szFriendlyName, camera_name))
        {
            device_index = i;
        }
        CoTaskMemFree(szFriendlyName);
    }

    hr = ppDevices[device_index]->ActivateObject(IID_PPV_ARGS(&pSource));
    if (FAILED(hr)) {
        std::cerr << "ActivateObject failed." << std::endl;
        return;
    }

    for (UINT32 i = 0; i < count; i++) {
        ppDevices[i]->Release();
    }
    CoTaskMemFree(ppDevices);

    hr = MFCreateSourceReaderFromMediaSource(pSource, pAttributes, &pReader);
    if (FAILED(hr)) {
        std::cerr << "MFCreateSourceReaderFromMediaSource failed." << std::endl;
        return;
    }


    try {
        virtual_output = new VirtualOutput(3840, 2160, 30);
    } catch (std::runtime_error err) {
        std::cout << err.what() << std::endl;
        return;
    }

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
                break;
            }

            BYTE* pData = nullptr;
            DWORD cbBuffer = 0;

            hr = pBuffer->Lock(&pData, nullptr, &cbBuffer);
            if (FAILED(hr)) {
                std::cerr << "Lock failed." << std::endl;
                break;
            }

            // Process the raw byte stream (pData, cbBuffer)

            //std::cout << "Received " << cbBuffer << " bytes of data." << std::endl;

            virtual_output->send(static_cast<uint8_t*>(pData));

            hr = pBuffer->Unlock();
            if (FAILED(hr)) {
                std::cerr << "Unlock failed." << std::endl;
                break;
            }
        }
    }

done:
    if(virtual_output) virtual_output->stop();
    delete virtual_output;

    SafeRelease(&pAttributes);

    for (DWORD i = 0; i < count; i++)
    {
        SafeRelease(&ppDevices[i]);
    }
    CoTaskMemFree(ppDevices);
    SafeRelease(&pSource);
}

int main() 
{
    int camera_device_index = 0;

    wchar_t buffer[MAX_PATH], drive[MAX_PATH] ,directory[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH); 
    _wsplitpath(buffer, drive, directory, NULL, NULL);
    std::wstring s_drive(drive), s_dir(directory);

    std::ofstream errorFile(s_drive + s_dir + L"ErrorFile.log");
    std::cerr.rdbuf(errorFile.rdbuf());

    std::wifstream camera_name_file(s_drive + s_dir + L"camera_name.txt");
    wchar_t camera_name[256];
    if(camera_name_file.good()){
        camera_name_file.getline(camera_name, 256);
    } else {
        std::cout << "camera_name.txt file not found" << std::endl;
    }
    camera_name_file.close();

    ReadCameraStream(camera_name);
    return 0;
}
