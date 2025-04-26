#pragma once

#include <vector>

#include "device_manager.hpp"

#include "ignite/core/types.hpp"

#include <Windows.h>
#include <dxgi1_5.h>
#include <dxgidebug.h>

#include <nvrhi/d3d12.h>
#include <nvrhi/validation.h>

namespace ignite
{
    struct DescriptorHeapAllocator
    {
        ID3D12DescriptorHeap *heap = nullptr;
        D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
        D3D12_CPU_DESCRIPTOR_HANDLE heapStartCpu;
        D3D12_GPU_DESCRIPTOR_HANDLE heapStartGpu;
        UINT heapHandleIncrement;
        std::vector<i32> freeIndices;

        void Create(ID3D12Device *device, ID3D12DescriptorHeap *heap);
        void Destroy();
        void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE *out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE *out_gpu_desc_handle);
        void Free(D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle);
    };

    class DeviceManager_DX12 final : public DeviceManager
    {
    public:
        nvrhi::RefCountPtr<IDXGIFactory2> m_DxgiFactory2;
        nvrhi::RefCountPtr<ID3D12Device> m_Device12;
        nvrhi::RefCountPtr<ID3D12CommandQueue> m_GraphicsQueue;
        nvrhi::RefCountPtr<ID3D12CommandQueue> m_ComputeQueue;
        nvrhi::RefCountPtr<ID3D12CommandQueue> m_CopyQueue;
        nvrhi::RefCountPtr<IDXGISwapChain3> m_SwapChain;
        nvrhi::RefCountPtr<ID3D12DescriptorHeap> m_RtvDescHeap;
        nvrhi::RefCountPtr<ID3D12DescriptorHeap> m_SrvDescHeap;
        DescriptorHeapAllocator m_SrvDescHeapAlloc;
        DXGI_SWAP_CHAIN_DESC1 m_SwapChainDesc{};
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC m_FullScreenDesc{};
        nvrhi::RefCountPtr<IDXGIAdapter> m_DxgiAdapter;
        HWND m_Hwnd = nullptr;
        bool m_TearingSupported = false;
        const i32 SRV_HEAP_SIZE = 64;

        std::vector<nvrhi::RefCountPtr<ID3D12Resource>> m_SwapChainBuffers;
        std::vector<nvrhi::TextureHandle> m_RhiSwapChainBuffers;
        nvrhi::RefCountPtr<ID3D12Fence> m_FrameFence;
        std::vector<HANDLE> m_FrameFenceEvents;
        uint64_t m_FrameCount = 1;
        nvrhi::DeviceHandle m_NvrhiDevice;
        std::string m_RendererString;

    public:
        static std::string GetAdapterName(DXGI_ADAPTER_DESC const &aDesc)
        {
            const size_t length = wcsnlen(aDesc.Description, _countof(aDesc.Description));
            std::string name;
            name.resize(length);
            WideCharToMultiByte(CP_ACP, 0, aDesc.Description, (int)length, name.data(), (int)name.size(), nullptr, nullptr);
            return name;
        }

        [[nodiscard]] const char *GetRendererString() const override
        {
            return m_RendererString.c_str();
        }

        [[nodiscard]] nvrhi::IDevice *GetDevice() const override
        {
            return m_NvrhiDevice;
        }

        void ReportLiveObjects() override;
        bool EnumerateAdapters(std::vector<AdapterInfo> &outAdapters) override;

        [[nodiscard]] nvrhi::GraphicsAPI GetGraphicsAPI() const override
        {
            return nvrhi::GraphicsAPI::D3D12;
        }

        void WaitForIdle() override;

        static DeviceManager_DX12 &GetInstance();

    protected:
        bool CreateInstanceInternal() override;
        bool CreateDevice() override;
        bool CreateSwapChain() override;
        void DestroyDeviceAndSwapChain() override;
        void ResizeSwapChain() override;
        nvrhi::ITexture *GetCurrentBackBuffer() override;
        nvrhi::ITexture *GetBackBuffer(u32 index) override;
        u32 GetCurrentBackBufferIndex() override;
        u32 GetBackBufferCount() override;
        bool BeginFrame() override;
        bool Present() override;
        void Destroy() override;
        bool CreateRenderTargets();
        void ReleaseRenderTargets();
    };
}