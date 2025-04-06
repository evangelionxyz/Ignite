#pragma once

#include "layer.hpp"
#include <list>

class LayerStack
{
public:
    LayerStack() = default;

    void PushLayer(Layer *layer)
    {
        layer->OnAttach();
        m_Layers.emplace(m_Layers.end(), layer);
    }

    void PopLayer(Layer *layer)
    {
        layer->OnDetach();
        m_Layers.remove(layer);

        delete layer;
    }

    void Destroy()
    {
        for (Layer *layer : m_Layers)
        {
            layer->OnDetach();
            delete layer;
        }
        m_Layers.clear();
    }

    std::list<Layer*>::const_iterator Begin() { return m_Layers.begin(); }
    std::list<Layer*>::const_iterator End() { return m_Layers.end(); }
    std::list<Layer*>::const_reverse_iterator Rbegin() { return m_Layers.rbegin(); }
    std::list<Layer*>::const_reverse_iterator Rend() { return m_Layers.rend(); }

private:
    std::list<Layer *> m_Layers;
};
