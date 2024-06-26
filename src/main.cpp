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
#include <d3d11.h>
#include <d3d11_4.h>

#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")
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



HRESULT ConfigureDXGIManager(CComPtr<IMFDXGIDeviceManager>& D3DMgr) {
    HRESULT hr;
    CComPtr<ID3D11Device> D3DDev;
    UINT mgrRToken;

    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,  D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1 };
    if (SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
        levels, sizeof(levels) / sizeof(*levels), D3D11_SDK_VERSION, &D3DDev, NULL, NULL)))
    {
        if (FAILED(hr = MFCreateDXGIDeviceManager(&mgrRToken, &D3DMgr))) {
            std::cerr << "MFCreateDXGIDeviceManager failed." << std::endl;
            return hr;
        }

        // NOTE: Getting ready for multi-threaded operation
        CComPtr<ID3D11Multithread> D3DDevMT;
        if (SUCCEEDED(D3DDev->QueryInterface(IID_PPV_ARGS(&D3DDevMT))))
        {
            D3DDevMT->SetMultithreadProtected(TRUE);
            if (SUCCEEDED(D3DMgr->ResetDevice(D3DDev, mgrRToken)))
            {
                // Log adapter description
                CComPtr<IDXGIDevice> dxgiDevice;
                if (SUCCEEDED(D3DDev->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice)))) {
                    CComPtr<IDXGIAdapter> adapter;
                    if (SUCCEEDED(dxgiDevice->GetAdapter(&adapter))) {
                        DXGI_ADAPTER_DESC desc;
                        if (SUCCEEDED(adapter->GetDesc(&desc))) {
                            std::wstring name(desc.Description);
                            std::wcout << "Using D3D11 video acceleration on GPU device: " << name << std::endl;
                        }
                    }
                }
                return S_OK;
            }
        }
    }

    return S_OK;
}

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


bool SetVideoCaptureResolution(IMFSourceReader* pReader, UINT32 width, UINT32 height) {
    CComPtr<IMFMediaType> pType;
    DWORD i = 0;

    while (SUCCEEDED(pReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, &pType))) {
        GUID subtype = { 0 };
        pType->GetGUID(MF_MT_SUBTYPE, &subtype);

        if (subtype == MFVideoFormat_NV12) {
            UINT32 currentWidth = 0, currentHeight = 0;
            MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &currentWidth, &currentHeight);

            if (currentWidth == width && currentHeight == height) {
                HRESULT hr = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pType);
                if (SUCCEEDED(hr)) {
                    std::cout << "Successfully set the resolution to " << width << "x" << height << std::endl;
                    return true;
                } else {
                    std::cerr << "Failed to set media type." << std::endl;
                    ErrorDescription(hr);
                }
            }
        }

        pType.Release();
        i++;
    }

    std::cerr << "Desired resolution not found." << std::endl;
    return false;
}

void CameraLoop(const wchar_t *camera_name, 
                VirtualOutput &virtual_output, 
                CComPtr<IMFAttributes> &pAttributes,
                CComPtr<IMFMediaSource> &pSource,
                CComPtr<IMFSourceReader> &pReader,
                IMFActivate** &ppDevices)
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

    CComPtr<IMFDXGIDeviceManager> dxgiManager;
    hr = ConfigureDXGIManager(dxgiManager);
    if (FAILED(hr)) {
        std::cerr << "ConfigureDXGIManager failed." << std::endl;
        return;
    }
    hr = pAttributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, dxgiManager);
    if (FAILED(hr)) {
        std::cerr << "SetUnknown for DXGI Manager failed." << std::endl;
        return;
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

    if (!SetVideoCaptureResolution(pReader, 3840, 2160)){
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
}

void ReadCameraStream(const wchar_t *camera_name)
{
    while (true)
    {
        MediaFoundationInitializer mfInitializer;

        CComPtr<IMFAttributes> pAttributes;
        CComPtr<IMFMediaSource> pSource;
        CComPtr<IMFSourceReader> pReader;
        IMFActivate** ppDevices = nullptr;
        HRESULT hr = MFCreateAttributes(&pAttributes, 2);

        if (SUCCEEDED(hr)) {
            try {
                VirtualOutput virtual_output(3840, 2160, 30);
                CameraLoop(camera_name, virtual_output, pAttributes, pSource, pReader, ppDevices);
                virtual_output.stop();
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
    if (argc <= 1) {
        FreeConsole();
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

    ReadCameraStream(camera_name);
    return 0;
}
