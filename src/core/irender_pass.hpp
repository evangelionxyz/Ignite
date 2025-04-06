#pragma once

#include "types.hpp"
#include <nvrhi/nvrhi.h>

class DeviceManager;

class IRenderPass
{
public:
    explicit IRenderPass(DeviceManager *deviceManager)
        : m_DeviceManager(deviceManager)
    {
    }

    virtual ~IRenderPass() = default;
    virtual void SetLateWarpOptions() { }
    virtual bool ShouldRenderUnfocused() { return false; }
    virtual void Render(nvrhi::IFramebuffer *framebuffer) { }
    virtual void Animate(float fElapsedTimeSeconds) { }
    virtual void BackBufferResizing() { }
    virtual void BackBufferResized(const u32 width, const u32 heigth, const u32 sampleCount) { }
    virtual void Destroy() { }

    // Called before Animate() when a DPI change was created
    virtual void DisplayScaleChanged(float scaleX, float scaleY) { }

    virtual bool KeyboardUpdate(int key, int scancode, int action, int mods) { return false; }
    virtual bool KeyboardCharInput(unsigned int unicode, int modes) { return false; }
    virtual bool MousePosUpdate(double xpos, double ypos) { return false; }
    virtual bool MouseScrollUpdate(double xoffset, double yoffset) { return false; }
    virtual bool MouseButtonUpdate(int button, int action, int mods) { return false; }
    virtual bool JoystickButtonUpdate(int button, bool pressed) { return false; }
    virtual bool JoystickAxisUpdate(int axis, float value) { return false; }

    [[nodiscard]] DeviceManager *GetDeviceManager() const;
    [[nodiscard]] nvrhi::IDevice *GetDevice() const;
    [[nodiscard]] u32 GetFrameIndex() const;
protected:
    DeviceManager *m_DeviceManager;
};