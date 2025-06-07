#pragma once

#include "ignite/core/types.hpp"
#include "ignite/core/device/device_manager.hpp"
#include "imgui_nvrhi.hpp"
#include "ignite/core/input/event.hpp"
#include "ignite/core/layer.hpp"
#include "ignite/core/input/app_event.hpp"

#include "ignite/core/buffer.hpp"

#include <filesystem>
#include <optional>

namespace ignite
{
    class ShaderFactory;

    class GuiFont
    {
    public:
        GuiFont();
        GuiFont(f32 size);
        GuiFont(Buffer data, bool isCompressed, f32 size);

        bool HasFontData() const { return m_Data.Data != nullptr; }
        ImFont *GetScaledFont() const { return m_ImFont; }

    protected:
        friend class ImGuiLayer;

        Buffer m_Data;
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
        bool Init();
        void OnDetach();

        void BeginFrame();
        void EndFrame(nvrhi::IFramebuffer* framebuffer);

        void OnEvent(Event &event) override;
        bool OnFramebufferResize(FramebufferResizeEvent &event) const;

    private:
        Scope<ImGui_NVRHI> imguiNVRHI;
        Ref<GuiFont> m_Font;

        bool m_SupportExplicitDisplayScaling;
        bool m_BeginFrameCalled = false;
        DeviceManager *m_DeviceManager = nullptr;
    };
}