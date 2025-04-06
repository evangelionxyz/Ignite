#include "imgui_layer.hpp"
#include "core/vfs/vfs.hpp"

#include "core/logger.hpp"

#include <backends/imgui_impl_glfw.h>
#include <core/application.hpp>

#ifdef _WIN32
#include "core/device/device_manager_dx12.hpp"
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_dx12.h>
#endif

void RegisteredFont::CreateScaledFont(f32 displayScale)
{
    ImFontConfig fontConfig;
    fontConfig.SizePixels = m_SizeAtDefaultScale * displayScale;

    m_ImFont = nullptr;
    if (m_Data)
    {
        fontConfig.FontDataOwnedByAtlas = false;
        if (m_IsCompressed)
            m_ImFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(m_Data->Data(), static_cast<int>(m_Data->Size()), 0.0f, &fontConfig);
        else
            m_ImFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(const_cast<void *>(m_Data->Data()), int(m_Data->Size()), 0.0f, &fontConfig);
    }
    else if (m_IsDefault)
    {
        m_ImFont = ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);
    }

    if (m_ImFont)
    {
        ImGui::GetIO().Fonts->TexID = 0;
    }
}

void RegisteredFont::ReleaseScaledFont()
{
    m_ImFont = nullptr;
}

ImGuiLayer::ImGuiLayer(DeviceManager *deviceManager)
    : Layer("ImGuiLayer")
    , m_DeviceManager(deviceManager)
    , m_SupportExplicitDisplayScaling(deviceManager->GetDeviceParams().supportExplicitDisplayScaling)
{

    LOG_ASSERT(m_DeviceManager, "Invalid device manager");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    m_DefaultFont = CreateRef<RegisteredFont>(13.0f);
    m_Fonts.push_back(m_DefaultFont);

    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2{ 0.0f, 0.0f };
    style.FramePadding = ImVec2{ 6.0f, 3.0f };
    style.CellPadding = ImVec2{ 7.0f, 3.0f };
    style.ItemSpacing = ImVec2{ 4.0f, 3.0f };
    style.ItemInnerSpacing = ImVec2{ 6.0f, 3.0f };
    style.TouchExtraPadding = ImVec2{ 0.0f, 0.0f };
    style.IndentSpacing = 8;
    style.ScrollbarSize = 16.0f;
    style.GrabMinSize = 13;
    style.WindowBorderSize = 0;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 0;
    style.TabBorderSize = 0;
    style.WindowRounding = 0;
    style.ChildRounding = 0;
    style.FrameRounding = 0;
    style.PopupRounding = 0;
    style.ScrollbarRounding = 0;
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.AntiAliasedFill = true;
    style.AntiAliasedLines = true;
    style.AntiAliasedLinesUseTex = true;

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigViewportsNoDecoration = false;

    ImGui_ImplGlfw_InitForOther(m_DeviceManager->GetWindow(), true);
    //ImGui_ImplWin32_Init(glfwGetWin32Window(m_DeviceManager->GetWindow()));

    DeviceManager_DX12 &d3d12 = DeviceManager_DX12::GetInstance();
    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = d3d12.m_Device12;
    initInfo.CommandQueue = d3d12.m_GraphicsQueue;
    initInfo.NumFramesInFlight = m_DeviceManager->GetDeviceParams().maxFramesInFlight;
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    // Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
    // (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
    initInfo.SrvDescriptorHeap = d3d12.m_SrvDescHeap;
    initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) 
        { 
            return DeviceManager_DX12::GetInstance().m_SrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle);
        };
    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
        { 
            return DeviceManager_DX12::GetInstance().m_SrvDescHeapAlloc.Free(cpu_handle, gpu_handle); 
        };

    ImGui_ImplDX12_Init(&initInfo);
}

bool ImGuiLayer::Init(Ref<ShaderFactory> shaderFactory)
{
    imgui_nvrhi = CreateScope<ImGui_NVRHI>();
    return imgui_nvrhi->Init(m_DeviceManager->GetDevice(), shaderFactory);
}

Ref<RegisteredFont> ImGuiLayer::CreateFontFromFile(vfs::IFileSystem &fs, const std::filesystem::path &fontFile, f32 fontSize)
{
    auto fontData = fs.ReadFile(fontFile);

    if(!fontData)
        return CreateRef<RegisteredFont>();
    
    Ref<RegisteredFont> font = CreateRef<RegisteredFont>(fontData, false, fontSize);
    m_Fonts.push_back(font);

    return std::move(font);
}

Ref<RegisteredFont> ImGuiLayer::CreateFontFromMemoryInternal(void const *pData, size_t size, bool compressed, f32 fontSize)
{
    if (!pData || !size)
        return CreateRef<RegisteredFont>();

    // copy the font data into a blob to make the RegisteredFont object own it
    void *dataCopy = malloc(size);
    memcpy(dataCopy, pData, size);
    Ref<vfs::Blob> blob = CreateRef<vfs::Blob>(dataCopy, size);

    Ref<RegisteredFont> font = CreateRef<RegisteredFont>(blob, compressed, fontSize);
    m_Fonts.push_back(font);
    return std::move(font);
}

Ref<RegisteredFont> ImGuiLayer::CreateFontFromMemoryCompressed(void const *pData, size_t size, f32 fontSize)
{
    return CreateFontFromMemoryInternal(pData, size, true, fontSize);
}

void ImGuiLayer::OnEvent(Event &event)
{
    Layer::OnEvent(event);

    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<FramebufferResizeEvent>(BIND_CLASS_EVENT_FN(ImGuiLayer::OnFramebufferResize));
}

bool ImGuiLayer::OnFramebufferResize(FramebufferResizeEvent &event) const
{
    if (imgui_nvrhi)
        imgui_nvrhi->BackBufferResizing();

    if (!m_SupportExplicitDisplayScaling)
        return false;

    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->TexID = 0;

    for (auto &font : m_Fonts)
        font->ReleaseScaledFont();

    ImGui::GetStyle() = ImGui::GetStyle();
    ImGui::GetStyle().ScaleAllSizes(event.GetWidth());

    return true;
}

void ImGuiLayer::BeginFrame()
{
    if (!imgui_nvrhi || m_BeginFrameCalled)
        return;

    f32 scaleX, scaleY;
    m_DeviceManager->GetDPIScaleInfo(scaleX, scaleY);

    for (const auto &font : m_Fonts)
    {
        if (!font->GetScaledFont())
            font->CreateScaledFont(m_SupportExplicitDisplayScaling ? scaleX : 1.0f);
    }

    imgui_nvrhi->UpdateFontTexture();

    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    m_BeginFrameCalled = true;
}

void ImGuiLayer::EndFrame(nvrhi::IFramebuffer* framebuffer)
{
    ImGui::Render();
    imgui_nvrhi->Render(framebuffer);
    m_BeginFrameCalled = false;
}

void ImGuiLayer::BeginFullScreenWindow()
{
    ImGuiIO const &io = ImGui::GetIO();
    ImGui::SetNextWindowPos({0.0f, 0.0f}, ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        { io.DisplaySize.x / io.DisplayFramebufferScale.x, io.DisplaySize.y / io.DisplayFramebufferScale.y},
        ImGuiCond_Always
    );

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin(" ", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
}

void ImGuiLayer::DrawScreenCenteredText(const char *text)
{
    ImGuiIO const &io = ImGui::GetIO();
    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImGui::SetCursorPosX((io.DisplaySize.x / io.DisplayFramebufferScale.x - textSize.x) * 0.5f);
    ImGui::SetCursorPosY((io.DisplaySize.y / io.DisplayFramebufferScale.y - textSize.y) * 0.5f);
    ImGui::TextUnformatted(text);
}

void ImGuiLayer::EndFullScreenWindow()
{
    ImGui::End();
    ImGui::PopStyleVar();
}

void ImGuiLayer::Destroy()
{
    m_DeviceManager->WaitForIdle();

    m_DefaultFont = nullptr;
    m_Fonts.clear();

    imgui_nvrhi->Shutdown();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
