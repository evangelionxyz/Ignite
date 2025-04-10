#pragma once

#include "ignite/core/types.hpp"
#include "ignite/core/device/device_manager.hpp"
#include "imgui_nvrhi.hpp"
#include "ignite/core/input/event.hpp"
#include "ignite/core/layer.hpp"
#include "ignite/core/input/app_event.hpp"

#include <filesystem>
#include <optional>

namespace vfs
{
    class IBlob;
    class IFileSystem;
}

class ShaderFactory;

class RegisteredFont
{
public:
    RegisteredFont()
        : m_IsDefault(false), m_IsCompressed(false), m_SizeAtDefaultScale(0.0f)
    {
    }

    RegisteredFont(f32 size)
        : m_IsDefault(true), m_IsCompressed(false), m_SizeAtDefaultScale(0.0f)
    {
    }

    RegisteredFont(Ref<vfs::IBlob> data, bool isCompressed, f32 size)
        : m_Data(data), m_IsDefault(false), m_IsCompressed(isCompressed), m_SizeAtDefaultScale(size)
    {
    }

    bool HasFontData() const { return m_Data != nullptr; }
    ImFont *GetScaledFont() const { return m_ImFont; }

protected:
    friend class ImGuiLayer;

    Ref<vfs::IBlob> m_Data;
    bool const m_IsDefault;
    bool const m_IsCompressed;
    f32 const m_SizeAtDefaultScale;
    ImFont *m_ImFont = nullptr;

    void CreateScaledFont(f32 displayScale);
    void ReleaseScaledFont();
};

class ImGuiLayer : public Layer
{
public:
    virtual ~ImGuiLayer() = default;

    ImGuiLayer(DeviceManager *deviceManager);
    bool Init(Ref<ShaderFactory> shaderFactory);
    void OnDetach();
    
    void BeginFrame();
    void EndFrame(nvrhi::IFramebuffer* framebuffer);
    
    void OnEvent(Event &event) override;
    bool OnFramebufferResize(FramebufferResizeEvent &event) const;
    
    Ref<RegisteredFont> CreateFontFromFile(vfs::IFileSystem &fs, const std::filesystem::path &fontFile, f32 fontSize);
    Ref<RegisteredFont> CreateFontFromMemoryCompressed(void const *pData, size_t size, f32 fontSize);
    Ref<RegisteredFont> GetDefaultFont() { return m_DefaultFont; }
    
private:
    void BeginFullScreenWindow();
    void DrawScreenCenteredText(const char *text);
    void EndFullScreenWindow();

    Scope<ImGui_NVRHI> imgui_nvrhi;
    std::vector<Ref<RegisteredFont>> m_Fonts;
    Ref<RegisteredFont> m_DefaultFont;
    bool m_SupportExplicitDisplayScaling;
    bool m_BeginFrameCalled = false;
    DeviceManager *m_DeviceManager = nullptr;
private:
    Ref<RegisteredFont> CreateFontFromMemoryInternal(void const *pData, size_t size, bool compressed, f32 fontSize);
};