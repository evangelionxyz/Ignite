#pragma once

#include "layer.hpp"
#include <list>

namespace ignite
{
    class LayerStack
    {
    public:
        LayerStack() = default;

        void PushLayer(Layer *layer)
        {
            m_Layers.emplace(m_Layers.end(), layer);
        }

        void PopLayer(Layer *layer)
        {
            m_Layers.remove(layer);
            delete layer;
        }

        std::list<Layer*>::const_iterator begin() { return m_Layers.begin(); }
        std::list<Layer*>::const_iterator end() { return m_Layers.end(); }
        std::list<Layer*>::const_reverse_iterator rbegin() { return m_Layers.rbegin(); }
        std::list<Layer*>::const_reverse_iterator rend() { return m_Layers.rend(); }

    private:
        std::list<Layer *> m_Layers;
    };
}
