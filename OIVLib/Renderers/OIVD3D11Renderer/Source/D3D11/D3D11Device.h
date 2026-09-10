#pragma once
#include <windows.h>
#include <d3dcommon.h>
#include <d3d11_2.h>
#include <dxgi1_4.h>
#include "D3D11Common.h"
#include "D3D11Error.h"
#include <cstdlib>
#include <stdexcept>
#include <Interfaces/RendererOptions.h>
#include <LLUtils/Warnings.h>

namespace OIV
{
    class D3D11Device
    {
    public:
        ID3D11DeviceContext* GetContext()  const
        {
            return fD3dContext.Get();
        }

        ID3D11Device* GetdDevice() const { return fD3dDevice.Get(); }

        int GetSelectedGPUIndex() const { return fGpuIndex; }

        IDXGIAdapter* GetAdapter() const { return fD3dAdapter.Get(); }

        DXGI_ADAPTER_DESC GetAdapterDesc() const
        {
            DXGI_ADAPTER_DESC desc = {};
            if (fD3dAdapter != nullptr)
                fD3dAdapter->GetDesc(&desc);
            return desc;
        }

        LARGE_INTEGER GetDriverVersion() const
        {
            LARGE_INTEGER version{};
            if (fD3dAdapter != nullptr && FAILED(fD3dAdapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &version)))
                version = {};
            return version;
        }

        IDXGISwapChain* GetSwapChain() const { return fD3dSwapChain.Get(); }

        void Create(HWND hwnd, int adapterIndex, const char* adapterName)
        {
            fHWND                               = hwnd;
            D3D_FEATURE_LEVEL requestedLevels[] = {D3D_FEATURE_LEVEL_11_0};

            UINT createFlags = 0;
            createFlags |= D3D11_CREATE_DEVICE_SINGLETHREADED;

#ifdef _DEBUG
            createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

            ComPtr<IDXGIAdapter1> selected;
            if (adapterIndex >= 0 || adapterName != nullptr)
            {
                ComPtr<IDXGIFactory1> factory;
                D3D11Error::HandleDeviceError(CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf())),
                                              "Could not enumerate D3D11 adapters");
                for (UINT index = 0;; ++index)
                {
                    ComPtr<IDXGIAdapter1> adapter;
                    const HRESULT enumeration = factory->EnumAdapters1(index, adapter.GetAddressOf());
                    if (enumeration == DXGI_ERROR_NOT_FOUND)
                        break;
                    D3D11Error::HandleDeviceError(enumeration, "Could not enumerate D3D11 adapters");
                    bool matches = adapterIndex >= 0 && index == static_cast<UINT>(adapterIndex);
                    if (adapterName != nullptr)
                    {
                        DXGI_ADAPTER_DESC1 desc{};
                        D3D11Error::HandleDeviceError(adapter->GetDesc1(&desc), "Could not read adapter name");
                        const std::wstring_view description(desc.Description);
                        const int size = WideCharToMultiByte(CP_UTF8, 0, description.data(),
                                                             static_cast<int>(description.size()), nullptr, 0, nullptr,
                                                             nullptr);
                        std::string name(static_cast<size_t>(size), '\0');
                        WideCharToMultiByte(CP_UTF8, 0, description.data(), static_cast<int>(description.size()),
                                            name.data(), size, nullptr, nullptr);
                        matches = detail::AdapterNameMatches(adapterName, name);
                    }
                    if (matches)
                    {
                        selected  = std::move(adapter);
                        fGpuIndex = static_cast<int>(index);
                        break;
                    }
                }
                if (selected == nullptr)
                    throw std::invalid_argument("Requested D3D11 adapter was not found");
            }
            HRESULT res = D3D11CreateDevice(selected.Get(),
                                            selected != nullptr ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
                                            nullptr, createFlags, requestedLevels, 1, D3D11_SDK_VERSION,
                                            fD3dDevice.GetAddressOf(), nullptr, fD3dContext.GetAddressOf());

            if (FAILED(res) && selected == nullptr)
            {
                res = D3D11CreateDevice(
                    nullptr
                    , D3D_DRIVER_TYPE_WARP
                    , nullptr
                    , createFlags
                    , requestedLevels
                    , sizeof(requestedLevels) / sizeof(D3D_FEATURE_LEVEL)
                    , D3D11_SDK_VERSION
                    , fD3dDevice.GetAddressOf()
                    , nullptr
                    , fD3dContext.GetAddressOf());
            }

            if (FAILED(res) && selected != nullptr)
                throw std::runtime_error("The requested adapter cannot create a Direct3D 11 device");
            if (FAILED(res))
                D3D11Error::HandleDeviceError(res, "Could not create D3D11 device");
                
            

				ComPtr<IDXGIDevice> dxgiDevice;
				ComPtr<IDXGIAdapter> dxgiAdapter;
				ComPtr<IDXGIFactory2>  dxgiFactory;
LLUTILS_DISABLE_WARNING_PUSH
LLUTILS_DISABLE_WARNING_LANGUAGE_EXTENSION
				if (SUCCEEDED(fD3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgiDevice.GetAddressOf()))))
					if (SUCCEEDED(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(dxgiAdapter.GetAddressOf()))))
						if (SUCCEEDED(dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(dxgiFactory.GetAddressOf()))))
						{
                            fD3dAdapter = dxgiAdapter;
						}

				if (dxgiFactory == nullptr)
					LL_EXCEPTION(LLUtils::Exception::ErrorCode::RuntimeError, "This software requires windows 7 with platform update or higher");


                DXGI_SWAP_CHAIN_DESC1 scd{};

                ComPtr<IDXGIFactory4>  dxgiFactory4;
                if (SUCCEEDED(dxgiFactory->QueryInterface(__uuidof(IDXGIFactory4), reinterpret_cast<void**>(dxgiFactory4.GetAddressOf()))))
                {
                    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
                    scd.Scaling = DXGI_SCALING_NONE;
                }
                else
                {
                    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
                    scd.Scaling = DXGI_SCALING_STRETCH;
                }

LLUTILS_DISABLE_WARNING_POP

                scd.BufferCount = 2;
                scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                
                scd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
                scd.Width = 1280;
                scd.Height = 800;
                scd.Stereo = false;
                scd.SampleDesc.Count = 1;
                scd.SampleDesc.Quality = 0;
                scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                scd.Flags = static_cast<UINT>(0);


				dxgiFactory->CreateSwapChainForHwnd(fD3dDevice.Get(), fHWND, &scd, nullptr, nullptr, fD3dSwapChain.GetAddressOf());
				dxgiFactory->MakeWindowAssociation(fHWND, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);


        OIV_D3D_SET_OBJECT_NAME(fD3dDevice, "D3D11 device");
        OIV_D3D_SET_OBJECT_NAME(fD3dSwapChain, "D3D11 swap chain");
        OIV_D3D_SET_OBJECT_NAME(fD3dContext, "D3D11 context");
        }

    private:
        HWND fHWND = nullptr;
        int fGpuIndex = -1;
        ComPtr<IDXGISwapChain1> fD3dSwapChain;
        ComPtr<ID3D11DeviceContext> fD3dContext;
        ComPtr<ID3D11Device> fD3dDevice;
        ComPtr<IDXGIAdapter> fD3dAdapter;
    };

}
