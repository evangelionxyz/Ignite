#include "imgui_renderer.hpp"
#include "core/vfs/vfs.hpp"

#include "core/logger.hpp"

#include <backends/imgui_impl_glfw.h>

#ifdef _WIN32
#include "core/device_manager_dx12.hpp"
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
            m_ImFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF((void *)m_Data->Data(), int(m_Data->Size()), 0.0f, &fontConfig);
        else
            m_ImFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void *)m_Data->Data(), int(m_Data->Size()), 0.0f, &fontConfig);
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

ImGui_Renderer::ImGui_Renderer(DeviceManager *deviceManager)
    : IRenderPass(deviceManager)
    , m_SupportExplicitDisplayScaling(deviceManager->GetDeviceParams().supportExplicitDisplayScaling)
{
    
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
    initInfo.NumFramesInFlight = m_DeviceManager->GetDeviceParams().maxFramesInFligth;
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

bool ImGui_Renderer::LoadShaders(Ref<ShaderFactory> shaderFactory)
{
    imgui_nvrhi = CreateScope<ImGui_NVRHI>();
    return imgui_nvrhi->Init(GetDevice(), shaderFactory);
}

Ref<RegisteredFont> ImGui_Renderer::CreateFontFromFile(vfs::IFileSystem &fs, const std::filesystem::path &fontFile, f32 fontSize)
{
    auto fontData = fs.ReadFile(fontFile);

    if(!fontData)
        return CreateRef<RegisteredFont>();
    
    Ref<RegisteredFont> font = CreateRef<RegisteredFont>(fontData, false, fontSize);
    m_Fonts.push_back(font);

    return std::move(font);
}

Ref<RegisteredFont> ImGui_Renderer::CreateFontFromMemoryInternal(void const *pData, size_t size, bool compressed, f32 fontSize)
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

Ref<RegisteredFont> ImGui_Renderer::CreateFontFromMemoryCompressed(void const *pData, size_t size, f32 fontSize)
{
    return CreateFontFromMemoryInternal(pData, size, true, fontSize);
}

void ImGui_Renderer::Animate(f32 elapsedTimeInSeconds)
{
    if (!imgui_nvrhi || m_BeginFrameCalled)
        return;

    f32 scaleX, scaleY;
    m_DeviceManager->GetDPIScaleInfo(scaleX, scaleY);

    for (auto &font : m_Fonts)
    {
        if (!font->GetScaledFont())
            font->CreateScaledFont(m_SupportExplicitDisplayScaling ? scaleX : 1.0f);
    }

    imgui_nvrhi->UpdateFontTexture();

    i32 w, h;
    m_DeviceManager->GetWindowDimensions(w, h);

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(float(w), float(h));
    if (!m_SupportExplicitDisplayScaling)
    {
        io.DisplayFramebufferScale.x = scaleX;
        io.DisplayFramebufferScale.y = scaleY;
    }

    io.KeyCtrl = io.KeysData[GLFW_KEY_LEFT_CONTROL].Down || io.KeysData[GLFW_KEY_RIGHT_CONTROL].Down;
    io.KeyShift = io.KeysData[GLFW_KEY_LEFT_SHIFT].Down || io.KeysData[GLFW_KEY_LEFT_SHIFT].Down;
    io.KeyAlt = io.KeysData[GLFW_KEY_LEFT_ALT].Down || io.KeysData[GLFW_KEY_LEFT_ALT].Down;
    io.KeySuper = io.KeysData[GLFW_KEY_LEFT_SUPER].Down || io.KeysData[GLFW_KEY_LEFT_SUPER].Down;
    
    io.DeltaTime = elapsedTimeInSeconds;
    io.MouseDrawCursor = false;

    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    m_BeginFrameCalled = true;
}

void ImGui_Renderer::Render(nvrhi::IFramebuffer *framebuffer)
{
    if (!imgui_nvrhi || !m_BeginFrameCalled)
        return;

    RenderGui();

    ImGui::Render();
    imgui_nvrhi->Render(framebuffer);
    m_BeginFrameCalled = false;
}

void ImGui_Renderer::BackBufferResizing()
{
    if (imgui_nvrhi)
        imgui_nvrhi->BackBufferResizing();
}

void ImGui_Renderer::DisplayScaleChanged(f32 scaleX, f32 scaleY)
{
    if (!m_SupportExplicitDisplayScaling)
        return;
    
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->TexID = 0;

    for (auto &font : m_Fonts)
        font->ReleaseScaledFont();

    ImGui::GetStyle() = ImGui::GetStyle();
    ImGui::GetStyle().ScaleAllSizes(scaleX);
}

void ImGui_Renderer::BeginFullScreenWindow()
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

void ImGui_Renderer::DrawScreenCenteredText(const char *text)
{
    ImGuiIO const &io = ImGui::GetIO();
    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImGui::SetCursorPosX((io.DisplaySize.x / io.DisplayFramebufferScale.x - textSize.x) * 0.5f);
    ImGui::SetCursorPosY((io.DisplaySize.y / io.DisplayFramebufferScale.y - textSize.y) * 0.5f);
    ImGui::TextUnformatted(text);
}

void ImGui_Renderer::EndFullScreenWindow()
{
    ImGui::End();
    ImGui::PopStyleVar();
}

void ImGui_Renderer::Destroy()
{
    m_DeviceManager->WaitForIdle();

    m_DefaultFont = nullptr;
    m_Fonts.clear();

    imgui_nvrhi->Shutdown();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
