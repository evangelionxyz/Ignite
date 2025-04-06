#pragma once

#include "core/types.hpp"
#include "core/device_manager.hpp"
#include "core/irender_pass.hpp"
#include "imgui_nvrhi.hpp"

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

    ImFont *GetScaledFont() { return m_ImFont; }

protected:
    friend class ImGui_Renderer;

    Ref<vfs::IBlob> m_Data;
    bool const m_IsDefault;
    bool const m_IsCompressed;
    f32 const m_SizeAtDefaultScale;
    ImFont *m_ImFont = nullptr;

    void CreateScaledFont(f32 displayScale);
    void ReleaseScaledFont();
};

class ImGui_Renderer : public IRenderPass
{
public:
    ImGui_Renderer(DeviceManager *deviceManager);

    bool LoadShaders(Ref<ShaderFactory> shaderFactory);
    void Destroy() override;

    Ref<RegisteredFont> CreateFontFromFile(vfs::IFileSystem &fs, const std::filesystem::path &fontFile, f32 fontSize);
    Ref<RegisteredFont> CreateFontFromMemoryCompressed(void const *pData, size_t size, f32 fontSize);
    Ref<RegisteredFont> GetDefaultFont() { return m_DefaultFont; }

    virtual void Animate(f32 elapsedTimeSeconds) override;
    virtual void Render(nvrhi::IFramebuffer* framebuffer) override;
    virtual void BackBufferResizing() override;
    virtual void DisplayScaleChanged(f32 scaleX, f32 scaleY) override;

protected:
    virtual void RenderGui() = 0;
    void BeginFullScreenWindow();
    void DrawScreenCenteredText(const char *text);
    void EndFullScreenWindow();

protected:
    Scope<ImGui_NVRHI> imgui_nvrhi;
    std::vector<Ref<RegisteredFont>> m_Fonts;
    Ref<RegisteredFont> m_DefaultFont;
    bool m_SupportExplicitDisplayScaling;
    bool m_BeginFrameCalled = false;
private:
    Ref<RegisteredFont> CreateFontFromMemoryInternal(void const *pData, size_t size, bool compressed, f32 fontSize);
};