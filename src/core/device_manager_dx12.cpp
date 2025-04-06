#include <string>
#include <algorithm>
#include <vector>

#include "device_manager.hpp"
#include "device_manager_dx12.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <dxgi1_5.h>
#include <dxgidebug.h>

#include <nvrhi/d3d12.h>
#include <nvrhi/validation.h>
#include <sstream>
#include <print>

#include "logger.hpp"

using nvrhi::RefCountPtr;

#define HR_RETURN(hr) if (FAILED(hr)) return false
static bool IsNvDevice(const UINT ID) { return ID == 0x10DE; }

void DescriptorHeapAllocator::Create(ID3D12Device *device, ID3D12DescriptorHeap *heap)
{
    this->heap = heap;
    D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
    heapType = desc.Type;
    heapStartCpu = heap->GetCPUDescriptorHandleForHeapStart();
    heapStartGpu = heap->GetGPUDescriptorHandleForHeapStart();
    heapHandleIncrement = device->GetDescriptorHandleIncrementSize(heapType);
    freeIndices.resize(i32(desc.NumDescriptors));

    for (i32 i = desc.NumDescriptors; i > 0; --i)
        freeIndices.push_back(i - 1);
}

void DescriptorHeapAllocator::Destroy()
{
    heap = nullptr;
    freeIndices.clear();
}

void DescriptorHeapAllocator::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE *out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE *out_gpu_desc_handle)
{
    assert(freeIndices.size() > 0);
    int idx = freeIndices.back();
    freeIndices.pop_back();
    out_cpu_desc_handle->ptr = heapStartCpu.ptr + (idx * heapHandleIncrement);
    out_gpu_desc_handle->ptr = heapStartGpu.ptr + (idx * heapHandleIncrement);
}

void DescriptorHeapAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle)
{
    int cpu_idx = (int)((cpu_desc_handle.ptr - heapStartCpu.ptr) / heapHandleIncrement);
    int gpu_idx = (int)((gpu_desc_handle.ptr - heapStartGpu.ptr) / heapHandleIncrement);
    assert(cpu_idx == gpu_idx);
    freeIndices.push_back(cpu_idx);
}

static bool MoveWindowOntoAdapter(IDXGIAdapter *targetAdapter, RECT &rect)
{
    assert(targetAdapter != NULL);

    HRESULT hr = S_OK;
    unsigned int outputNo = 0;
    while (SUCCEEDED(hr))
    {
        RefCountPtr<IDXGIOutput> pOutput;
        hr = targetAdapter->EnumOutputs(outputNo++, &pOutput);

        if (SUCCEEDED(hr) && pOutput)
        {
            DXGI_OUTPUT_DESC outputDesc;
            pOutput->GetDesc(&outputDesc);
            const RECT desktop = outputDesc.DesktopCoordinates;
            const int centerX = (int)desktop.left + (int)(desktop.right - desktop.left) / 2;
            const int centerY = (int)desktop.top + (int)(desktop.bottom - desktop.top) / 2;
            const int winW = rect.right - rect.left;
            const int winH = rect.bottom - rect.top;
            const int left = centerX - winW / 2;
            const int right = left + winW;
            const int top = centerY - winH / 2;
            const int bottom = top + winH;
            rect.left = std::max(left, (int)desktop.left);
            rect.right = std::min(right, (int)desktop.right);
            rect.top = std::max(top, (int)desktop.top);
            rect.bottom = std::min(bottom, (int)desktop.bottom);

            return true;
        }
    }

    return false;
}

static DeviceManager_DX12 *s_Instance = nullptr;

void DeviceManager_DX12::ReportLiveObjects()
{
    RefCountPtr<IDXGIDebug> pDebug;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug));

    if (pDebug)
    {
        DXGI_DEBUG_RLO_FLAGS flags = (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_DETAIL);
        HRESULT hr = pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, flags);
        if (FAILED(hr))
        {
            printf("ReportLiveObjects failed, HRESULT = 0x%08x", hr);
        }
    }
}

bool DeviceManager_DX12::CreateInstanceInternal()
{
    if (!m_DxgiFactory2)
    {
        HRESULT hr = CreateDXGIFactory2(m_DeviceParams.enableDebugRuntime ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&m_DxgiFactory2));
        LOG_ASSERT(hr == S_OK, "Failed to create DXGIFactory2, for more info, get log from debug D3D Runtime");
        if (hr != S_OK)
            return false;
    }
    return true;
}

bool DeviceManager_DX12::EnumerateAdapters(std::vector<AdapterInfo>& outAdapters)
{
    if (!m_DxgiFactory2)
        return false;

    outAdapters.clear();

    while (true)
    {
        RefCountPtr<IDXGIAdapter> adapter;
        HRESULT hr = m_DxgiFactory2->EnumAdapters(uint32_t(outAdapters.size()), &adapter);
        if (FAILED(hr))
            return true;

        DXGI_ADAPTER_DESC desc;
        hr = adapter->GetDesc(&desc);
        if (FAILED(hr))
            return false;

        AdapterInfo adapterInfo;
        adapterInfo.name = GetAdapterName(desc);
        adapterInfo.dxgiAdapter = adapter;
        adapterInfo.vendorID = desc.VendorId;
        adapterInfo.deviceID = desc.DeviceId;
        adapterInfo.dedicatedVideoMemory = desc.DedicatedVideoMemory;
        AdapterInfo::LUUID luid;
        static_assert(luid.size() == sizeof(desc.AdapterLuid));
        memcpy(luid.data(), &desc.AdapterLuid, luid.size());
        adapterInfo.luuid = luid;
        outAdapters.push_back(std::move(adapterInfo));
    }
}

bool DeviceManager_DX12::CreateDevice()
{
    if (m_DeviceParams.enableDebugRuntime)
    {
        RefCountPtr<ID3D12Debug> pDebug;
        const HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug));
        if (SUCCEEDED(hr)) pDebug->EnableDebugLayer();
        else printf("Cannot enable DX12 debug runtim, ID3D12Debug is not available.");
    }

    if (m_DeviceParams.enableGPUValidation)
    {
        RefCountPtr<ID3D12Debug3> debugController3;
        const HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController3));
        if (SUCCEEDED(hr)) debugController3->SetEnableGPUBasedValidation(true);
        else printf("Cannot enable GPU-based validation, ID3D12Debug3 is not available.");
    }

    int adapterIndex = m_DeviceParams.adapterIndex;

    if (adapterIndex < 0) adapterIndex = 0;

    if (FAILED(m_DxgiFactory2->EnumAdapters(adapterIndex, &m_DxgiAdapter)))
    {
        if (adapterIndex == 0) printf("Cannot find any DXGI adapters in the system.");
        else printf("The specified DXGI adatpter %d does not exists.", adapterIndex);
        return false;
    }

    {
        DXGI_ADAPTER_DESC aDesc;
        m_DxgiAdapter->GetDesc(&aDesc);
        m_RendererString = GetAdapterName(aDesc);
        m_IsNvidia = IsNvDevice(aDesc.VendorId);
    }

    HRESULT hr = D3D12CreateDevice(
        m_DxgiAdapter,
        m_DeviceParams.featureLevel,
        IID_PPV_ARGS(&m_Device12)
    );

    if (FAILED(hr))
    {
        printf("DX12CreateDevice failed, error HRESULT = 0x%08x", hr);
        return false;
    }

    if (m_DeviceParams.enableDebugRuntime)
    {
        RefCountPtr<ID3D12InfoQueue> pInfoQueue;
        m_Device12->QueryInterface(&pInfoQueue);

        if (pInfoQueue)
        {
#ifdef _DEBUG
            if (m_DeviceParams.enableWarningAsErrors)
                pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D12_MESSAGE_ID disableMessageIDs[] =
            {
                D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_COMMAND_LIST_STATIC_DESCRIPTOR_RESOURCE_DIMENSION_MISMATCH,
            };

            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.pIDList = disableMessageIDs;
            filter.DenyList.NumIDs = std::size(disableMessageIDs);
            pInfoQueue->AddStorageFilterEntries(&filter);
        }
    }

    {

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = m_DeviceParams.maxFramesInFligth;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        hr = m_Device12->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_RtvDescHeap));
        LOG_ASSERT(hr == S_OK, "Faield to create RVT descriptor heap");
        HR_RETURN(hr);
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = SRV_HEAP_SIZE;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        hr = m_Device12->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_SrvDescHeap));
        LOG_ASSERT(hr == S_OK, "Faield to create SRV descriptor heap");
        HR_RETURN(hr);

        m_SrvDescHeapAlloc.Create(m_Device12, m_SrvDescHeap);

    }

    D3D12_COMMAND_QUEUE_DESC queueDesc;
    ZeroMemory(&queueDesc, sizeof(queueDesc));
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.NodeMask = 1;
    hr = m_Device12->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_GraphicsQueue));
    HR_RETURN(hr);
    m_GraphicsQueue->SetName(L"Graphics Queue");

    if (m_DeviceParams.enableComputeQueue)
    {
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        hr = m_Device12->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_ComputeQueue));
        HR_RETURN(hr);
        m_ComputeQueue->SetName(L"Compute Queue");
    }

    if (m_DeviceParams.enableCopyQueue)
    {
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        hr = m_Device12->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CopyQueue));
        HR_RETURN(hr);
        m_CopyQueue->SetName(L"Copy Queue");
    }

    nvrhi::d3d12::DeviceDesc deviceDesc;
    deviceDesc.errorCB = m_DeviceParams.messageCallback ? m_DeviceParams.messageCallback : &DefaultMessageCallback::GetInstance();
    deviceDesc.pDevice = m_Device12;
    deviceDesc.pGraphicsCommandQueue = m_GraphicsQueue;
    deviceDesc.pComputeCommandQueue = m_ComputeQueue;
    deviceDesc.pCopyCommandQueue =m_CopyQueue;
    deviceDesc.logBufferLifetime = m_DeviceParams.logBufferLifetime;
    deviceDesc.enableHeapDirectlyIndexed = m_DeviceParams.enableHeapDirectlyIndexed;

    m_NvrhiDevice = nvrhi::d3d12::createDevice(deviceDesc);
    if (m_DeviceParams.enableNvrhiValidationLayer)
        m_NvrhiDevice = nvrhi::validation::createValidationLayer(m_NvrhiDevice);

    return true;
}

bool DeviceManager_DX12::CreateSwapChain()
{
    UINT windowStyle = m_DeviceParams.startFullscreen
        ? (WS_POPUP | WS_SYSMENU | WS_VISIBLE)
        : m_DeviceParams.startMaximized
            ? (WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_MAXIMIZE)
            : (WS_OVERLAPPEDWINDOW | WS_VISIBLE);

    RECT rect = { 0, 0, LONG(m_DeviceParams.backBufferWidth), LONG(m_DeviceParams.backBufferHeight) };
    AdjustWindowRect(&rect, windowStyle, false);

    if (MoveWindowOntoAdapter(m_DxgiAdapter, rect))
    {
        glfwSetWindowPos(m_Window, rect.left, rect.top);
    }

    m_Hwnd = glfwGetWin32Window(m_Window);

    HRESULT hr = E_FAIL;

    RECT clientRect;
    GetClientRect(m_Hwnd, &clientRect);
    UINT width = clientRect.right - clientRect.left;
    UINT height = clientRect.bottom - clientRect.top;

    m_SwapChainDesc.Width = width;
    m_SwapChainDesc.Height = height;
    m_SwapChainDesc.SampleDesc.Count = m_DeviceParams.swapChainSampleCount;
    m_SwapChainDesc.SampleDesc.Quality = 0;
    m_SwapChainDesc.BufferUsage = m_DeviceParams.swapChainUsage;
    m_SwapChainDesc.BufferCount = m_DeviceParams.swapChainBufferCount;
    m_SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    m_SwapChainDesc.Flags = m_DeviceParams.allowModeSwitch ? DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH : 0;

    // Special processing for sRGB swap chain formats.
    // DXGI will not create a swap chain with an sRGB format, but its contents will be interpreted as sRGB.
    // So we need to use a non-sRGB format here, but store the true sRGB format for later framebuffer creation.

    switch (m_DeviceParams.swapChainFormat) // NOLINT(clang-diagnostic-switch-enum)
    {
    case nvrhi::Format::RGBA8_UNORM:
        m_SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
    case nvrhi::Format::BGRA8_UNORM:
        m_SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        break;
    default:
        m_SwapChainDesc.Format = nvrhi::d3d12::convertFormat(m_DeviceParams.swapChainFormat);
        break;
    }

    RefCountPtr<IDXGIFactory5> pDxgiFactory5;
    if (SUCCEEDED(m_DxgiFactory2->QueryInterface(&pDxgiFactory5)))
    {
        BOOL supported = 0;
        if (SUCCEEDED(pDxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &supported, sizeof(supported))))
            m_TearingSupported = (supported != 0);
    }

    if (m_TearingSupported)
    {
        m_SwapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    m_FullScreenDesc = {};
    m_FullScreenDesc.RefreshRate.Numerator = m_DeviceParams.refreshRate;
    m_FullScreenDesc.RefreshRate.Denominator = 1;
    m_FullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    m_FullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    m_FullScreenDesc.Windowed = !m_DeviceParams.startFullscreen;

    RefCountPtr<IDXGISwapChain1> pSwapChain1;
    hr = m_DxgiFactory2->CreateSwapChainForHwnd(m_GraphicsQueue, m_Hwnd, &m_SwapChainDesc, &m_FullScreenDesc, nullptr, &pSwapChain1);
    LOG_ASSERT(hr == S_OK, "Failed to create swapchain for HWND");
    HR_RETURN(hr);

    hr = pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_SwapChain));
    LOG_ASSERT(hr == S_OK, "Failed to Query swapchain interface");
    HR_RETURN(hr);

    if (!CreateRenderTargets())
        return false;
    
    hr = m_Device12->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_FrameFence));
    HR_RETURN(hr);

    for (UINT bufferIndex = 0; bufferIndex < m_SwapChainDesc.BufferCount; bufferIndex++)
    {
        m_FrameFenceEvents.push_back(CreateEvent(nullptr, false, true, nullptr));
    }

    return true;
}

void DeviceManager_DX12::DestroyDeviceAndSwapChain()
{
    m_RhiSwapChainBuffers.clear();
    m_RendererString.clear();

    ReleaseRenderTargets();

    m_NvrhiDevice = nullptr;

    for (auto fenceEvent : m_FrameFenceEvents)
    {
        WaitForSingleObject(fenceEvent, INFINITE);
        CloseHandle(fenceEvent);
    }

    m_FrameFenceEvents.clear();

    if (m_SwapChain)
    {
        m_SwapChain->SetFullscreenState(false, nullptr);
        m_SwapChain = nullptr;
    }

    m_SwapChainBuffers.clear();

    m_SrvDescHeapAlloc.Destroy();

    m_RtvDescHeap = nullptr;
    m_SrvDescHeap = nullptr;

    m_DxgiAdapter = nullptr;
    m_DxgiFactory2 = nullptr;

    m_FrameFence = nullptr;
    m_SwapChain = nullptr;
    m_GraphicsQueue = nullptr;
    m_ComputeQueue = nullptr;
    m_CopyQueue = nullptr;
    m_Device12 = nullptr;
}

bool DeviceManager_DX12::CreateRenderTargets()
{
    m_SwapChainBuffers.resize(m_SwapChainDesc.BufferCount);
    m_RhiSwapChainBuffers.resize(m_SwapChainDesc.BufferCount);

    for (UINT n = 0; n < m_SwapChainDesc.BufferCount; n++)
    {
        const HRESULT hr = m_SwapChain->GetBuffer(n, IID_PPV_ARGS(&m_SwapChainBuffers[n]));
        HR_RETURN(hr);

        nvrhi::TextureDesc textureDesc;
        textureDesc.width = m_DeviceParams.backBufferWidth;
        textureDesc.height = m_DeviceParams.backBufferHeight;
        textureDesc.sampleCount = m_DeviceParams.swapChainSampleCount;
        textureDesc.sampleQuality = m_DeviceParams.swapChainSampleQuality;
        textureDesc.format = m_DeviceParams.swapChainFormat;
        textureDesc.isRenderTarget = true;
        textureDesc.isUAV = false;
        textureDesc.initialState = nvrhi::ResourceStates::Present;
        textureDesc.keepInitialState = true;

        m_RhiSwapChainBuffers[n] = m_NvrhiDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D12_Resource, nvrhi::Object(m_SwapChainBuffers[n]), textureDesc);
    }

    return true;
}

void DeviceManager_DX12::WaitForIdle()
{
    if (m_NvrhiDevice)
        m_NvrhiDevice->waitForIdle();
}

void DeviceManager_DX12::ReleaseRenderTargets()
{
    if (m_NvrhiDevice)
    {
        m_NvrhiDevice->waitForIdle();
        m_NvrhiDevice->runGarbageCollection();
    }

    for (auto event : m_FrameFenceEvents)
        SetEvent(event);

    m_RhiSwapChainBuffers.clear();
    m_SwapChainBuffers.clear();
}

void DeviceManager_DX12::ResizeSwapChain()
{
    ReleaseRenderTargets();

    if (!m_NvrhiDevice || !m_SwapChain)
        return;
    
    const HRESULT hr = m_SwapChain->ResizeBuffers(
        m_DeviceParams.swapChainBufferCount, 
        m_DeviceParams.backBufferWidth, 
        m_DeviceParams.backBufferHeight, 
        m_SwapChainDesc.Format, 
        m_SwapChainDesc.Flags);

    if (FAILED(hr))
        LOG_ASSERT(false, "ResizeBuffers failed");
    
    bool ret = CreateRenderTargets();
    if (!ret)
        LOG_ASSERT(false, "CreateRenderTarget failed");
}

bool DeviceManager_DX12::BeginFrame()
{
    DXGI_SWAP_CHAIN_DESC1 newSwapChainDesc;
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC newFullScreenDesc;

    if (SUCCEEDED(m_SwapChain->GetDesc1(&newSwapChainDesc)) && SUCCEEDED(m_SwapChain->GetFullscreenDesc(&newFullScreenDesc)))
    {
        if (m_FullScreenDesc.Windowed != newFullScreenDesc.Windowed)
        {
            BackBufferResizing();
            m_FullScreenDesc = newFullScreenDesc;
            m_SwapChainDesc = newSwapChainDesc;
            m_DeviceParams.backBufferWidth = newSwapChainDesc.Width;
            m_DeviceParams.backBufferHeight = newSwapChainDesc.Height;

            if (newFullScreenDesc.Windowed)
            {
                glfwSetWindowMonitor(m_Window, nullptr, 
                    50, 50, 
                    newSwapChainDesc.Width, 
                    newSwapChainDesc.Height,
                    GLFW_DONT_CARE
                );
            }

            ResizeSwapChain();
            BackBufferResized();
        }
    }

    UINT bufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
    WaitForSingleObject(m_FrameFenceEvents[bufferIndex], INFINITE);
    return true;
}

nvrhi::ITexture *DeviceManager_DX12::GetCurrentBackBuffer()
{
    return m_RhiSwapChainBuffers[m_SwapChain->GetCurrentBackBufferIndex()];
}

nvrhi::ITexture *DeviceManager_DX12::GetBackBuffer(uint32_t index)
{
    if (index < m_RhiSwapChainBuffers.size())
        return m_RhiSwapChainBuffers[index];
    return nullptr;
}

uint32_t DeviceManager_DX12::GetCurrentBackBufferIndex()
{
    return m_SwapChain->GetCurrentBackBufferIndex();
}

uint32_t DeviceManager_DX12::GetBackBufferCount()
{
    return m_SwapChainDesc.BufferCount;
}

bool DeviceManager_DX12::Present()
{
    if (!m_WindowVisible)
        return true;

    UINT bufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

    UINT presentFlags = 0;
    if (!m_DeviceParams.vsyncEnable && m_FullScreenDesc.Windowed && m_TearingSupported)
        presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
    
    HRESULT hr = m_SwapChain->Present(m_DeviceParams.vsyncEnable ? 1 : 0, presentFlags);

    m_FrameFence->SetEventOnCompletion(m_FrameCount, m_FrameFenceEvents[bufferIndex]);
    m_GraphicsQueue->Signal(m_FrameFence, m_FrameCount);
    m_FrameCount++;

    return SUCCEEDED(hr);
}

void DeviceManager_DX12::Destroy()
{
    DeviceManager::Destroy();

    if (m_DeviceParams.enableDebugRuntime)
        ReportLiveObjects();
}

DeviceManager *DeviceManager::CreateD3D12()
{
    s_Instance = new DeviceManager_DX12();
    return s_Instance;
}

DeviceManager_DX12 &DeviceManager_DX12::GetInstance()
{
    return *s_Instance;
}

