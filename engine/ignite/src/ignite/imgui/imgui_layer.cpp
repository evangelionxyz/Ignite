#include "imgui_layer.hpp"
#include "ignite/core/application.hpp"
#include "ignite/core/logger.hpp"

#include <backends/imgui_impl_glfw.h>
#include <ImGuizmo.h>

#ifdef PLATFORM_WINDOWS
    #include <backends/imgui_impl_dx12.h>
    #include <backends/imgui_impl_win32.h>
    #include "ignite/core/device/device_manager_dx12.hpp"
#endif

#ifdef IGNITE_WITH_VULKAN
    #include <backends/imgui_impl_vulkan.h>
    #include "ignite/core/device/device_manager_vk.hpp"
#endif

#include "ignite/graphics/renderer.hpp"

namespace ignite
{

    GuiFont::GuiFont()
        : m_IsDefault(false)
        , m_IsCompressed(false)
        , m_SizeAtDefaultScale(0.0f)
    {
    }

    GuiFont::GuiFont(f32 size)
        : m_IsDefault(true)
        , m_IsCompressed(false)
        , m_SizeAtDefaultScale(0.0f)
    {
    }

    GuiFont::GuiFont(Buffer data, bool isCompressed, f32 size)
        : m_Data(data)
        , m_IsDefault(false)
        , m_IsCompressed(isCompressed)
        , m_SizeAtDefaultScale(size)
    {
    }

    void GuiFont::CreateScaledFont(f32 displayScale)
    {
        ImFontConfig fontConfig;
        fontConfig.SizePixels = m_SizeAtDefaultScale * displayScale;

        m_ImFont = nullptr;

        if (m_Data.Data)
        {
            fontConfig.FontDataOwnedByAtlas = false;
            m_ImFont = m_IsCompressed 
                ? ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(m_Data.Data, static_cast<int>(m_Data.Size), 0.0f, &fontConfig)
                : ImGui::GetIO().Fonts->AddFontFromMemoryTTF(m_Data.Data, static_cast<int>(m_Data.Size), 0.0f, &fontConfig);
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

    void GuiFont::ReleaseScaledFont()
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

        std::filesystem::path fontPath = "resources/fonts/segoeui.ttf";
        
        LOG_ASSERT(std::filesystem::exists(fontPath), "[ImGui Layer] font does not found");

        std::ifstream file(fontPath, std::ios::binary);
        if (file.is_open())
        {
            file.seekg(0, std::ios::end);
            u64 size = file.tellg();
            file.seekg(0, std::ios::beg);

            char *data = static_cast<char *>(malloc(size));
            LOG_ASSERT(data, "Out of memory");

            file.read(data, size);
            
            if (file.good())
            {
                m_Font = std::make_shared<GuiFont>(Buffer(data, size), false, 16);
            }
            else
            {
                free(data);
            }
            file.close();
        }

        ImGuiStyle &style = ImGui::GetStyle();

        ImVec4 *colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(22.0f/255.0f, 22.0f/255.0f, 22.0f/255.0f, 0.9f);
        colors[ImGuiCol_ChildBg] = ImVec4(22.0f/255.0f, 22.0f/255.0f, 22.0f/255.0f, 0.9f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.31f, 0.31f, 0.31f, 0.75f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.91f, 0.91f, 0.91f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(164.0f / 255.0f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
        colors[ImGuiCol_TabSelected] = ImVec4(116.0f / 255.0f, 0.0f, 0.0f, 1.00f);
        colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
        colors[ImGuiCol_TabDimmed] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
        colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.78f, 0.52f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.67f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.45f, 0.45f, 0.45f, 0.50f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 0.67f, 0.00f, 1.00f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.90f, 0.90f, 0.90f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.45f);

        style.WindowPadding = ImVec2{ 5.0f, 5.0f };
        style.FramePadding = ImVec2{ 3.0f, 5.0f };
        style.ItemSpacing = ImVec2{ 5.0f, 5.0f };
        style.ItemInnerSpacing = ImVec2{ 6.0f, 3.0f };
        style.CellPadding = ImVec2{ 2.0f, 4.0f };
        style.TouchExtraPadding = ImVec2{ 0.0f, 0.0f };
        style.WindowTitleAlign = ImVec2 { 0.5f, 0.5f };
        style.WindowBorderHoverPadding = 10.0f;
        style.WindowBorderSize = 0;
        style.ChildBorderSize = 1;
        style.PopupBorderSize = 1;

        style.IndentSpacing = 8;
        style.ScrollbarSize = 16.0f;
        style.GrabMinSize = 15.0f;
        style.FrameBorderSize = 0.0f;
        style.WindowRounding = 6.0f;
        style.TabRounding = 0.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.TabBorderSize = 1.0f;
        style.TabBarBorderSize = 1.0f;
        style.TabBarOverlineSize = 2.0f;
        style.TabCloseButtonMinWidthSelected = 1.0f;
        style.TabCloseButtonMinWidthUnselected = 1.0f;
        style.DockingSeparatorSize = 1.0f;

        style.WindowMenuButtonPosition = ImGuiDir_Right;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.AntiAliasedFill = true;
        style.AntiAliasedLines = true;
        style.AntiAliasedLinesUseTex = true;

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.ConfigViewportsNoDecoration = false;

        ImGui_ImplGlfw_InitForOther(m_DeviceManager->GetWindow(), true);

        switch (Renderer::GetGraphicsAPI())
        {
            case nvrhi::GraphicsAPI::D3D12:
            {
#ifdef PLATFORM_WINDOWS
                DeviceManager_DX12 &d3d12 = DeviceManager_DX12::GetInstance();
                ImGui_ImplDX12_InitInfo initInfo = {};
                initInfo.Device = d3d12.m_Device12;
                initInfo.CommandQueue = d3d12.m_GraphicsQueue;
                initInfo.NumFramesInFlight = m_DeviceManager->GetDeviceParams().maxFramesInFlight;
                initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
                initInfo.SrvDescriptorHeap = d3d12.m_SrvDescHeap;
                initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo *, D3D12_CPU_DESCRIPTOR_HANDLE *out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE *out_gpu_handle)
                {
                    return DeviceManager_DX12::GetInstance().m_SrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle);
                };
                initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo *, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
                {
                    return DeviceManager_DX12::GetInstance().m_SrvDescHeapAlloc.Free(cpu_handle, gpu_handle);
                };
                ImGui_ImplDX12_Init(&initInfo);
                break;
#endif
            }
        }
    }

    bool ImGuiLayer::Init()
    {
        imguiNVRHI = CreateScope<ImGui_NVRHI>();
        return imguiNVRHI->Init(m_DeviceManager->GetDevice());
    }

    void ImGuiLayer::OnEvent(Event &event)
    {
        Layer::OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<FramebufferResizeEvent>(BIND_CLASS_EVENT_FN(ImGuiLayer::OnFramebufferResize));
    }

    bool ImGuiLayer::OnFramebufferResize(FramebufferResizeEvent &event) const
    {
        if (imguiNVRHI)
            imguiNVRHI->BackBufferResizing();

        if (!m_SupportExplicitDisplayScaling)
            return false;

        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->TexID = 0;

        m_Font->ReleaseScaledFont();

        ImGui::GetStyle() = ImGui::GetStyle();
        ImGui::GetStyle().ScaleAllSizes(f32(event.GetWidth()));

        return true;
    }

    void ImGuiLayer::BeginFrame()
    {
        if (!imguiNVRHI || m_BeginFrameCalled)
            return;

        f32 scaleX, scaleY;
        m_DeviceManager->GetDPIScaleInfo(scaleX, scaleY);

        if (!m_Font->GetScaledFont())
        {
            m_Font->CreateScaledFont(m_SupportExplicitDisplayScaling ? scaleX : 1.0f);
        }

        imguiNVRHI->UpdateFontTexture();

        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        m_BeginFrameCalled = true;
    }

    void ImGuiLayer::EndFrame(nvrhi::IFramebuffer* framebuffer)
    {
        ImGui::Render();
        imguiNVRHI->Render(framebuffer);
        m_BeginFrameCalled = false;
    }

    void ImGuiLayer::OnDetach()
    {
        switch (Renderer::GetGraphicsAPI())
        {
            case nvrhi::GraphicsAPI::D3D12:
            {
#ifdef PLATFORM_WINDOWS
                ImGui_ImplDX12_Shutdown();
                break;
#endif
            }
        }

        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        imguiNVRHI->Shutdown();
    }
}