#pragma once

#include <string>
#include "types.hpp"
#include "input/event.hpp"

namespace nvrhi
{
    class IFramebuffer;
}

class Event;

class Layer
{
public:
    virtual ~Layer() = default;

    Layer() = default;

    Layer(const std::string &name)
        : m_Name(name)
    {
    }

    virtual void OnAttach() { }
    virtual void OnDetach() { }
    virtual void OnUpdate(float deltaTime) { }
    virtual void OnRender(nvrhi::IFramebuffer *framebuffer) { }
    virtual void OnEvent(Event &e) { }
    virtual void OnGuiRender() { }
    std::string GetName() { return m_Name; }

protected:
    std::string m_Name;
};
