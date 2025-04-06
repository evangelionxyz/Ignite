#pragma once

#include <string>
#include "types.hpp"
#include "input/event.hpp"

class Layer
{
public:
    virtual ~Layer() = default;
    explicit Layer(const std::string &name)
        : m_Name(name)
    {
    }

    virtual void OnAttach() { }
    virtual void OnDetach() { }
    virtual void OnUpdate(f32 deltaTime) { }
    virtual void OnRender(nvrhi::IFramebuffer *framebuffer) { }
    virtual void OnEvent(Event &e) { }
    virtual void OnGuiRender() { }
    inline std::string GetName() { return m_Name; }

protected:
    std::string m_Name;
};
