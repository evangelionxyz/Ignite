#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include "ignite/math/math.hpp"

namespace ignite {

    struct UIState
    {
        template<typename T>
        T *As()
        {
            return static_cast<T *>(this);
        }
    };

    struct UIStateColor : public UIState
    {
        glm::vec4 normal;
        glm::vec4 hover;

        UIStateColor() = default;

        UIStateColor(const glm::vec4 &color)
            : normal(color), hover(color)
        {
        }

        UIStateColor(const glm::vec4 &normal, const glm::vec4 &hover)
            : normal(normal), hover(hover)
        {
        }

        const glm::vec4 &GetColor(bool isHovered = false) const
        {
            if (isHovered) return hover;
            return normal;
        }
    };

    struct UIButtonDecorate : public UIState
    {
        UIStateColor fillColor;
        UIStateColor outlineColor;

        UIButtonDecorate()
        {
            fillColor = UIStateColor(
                { 0.2f, 0.2f, 0.2f, 1.0f },
                { 0.43f, 0.43f, 0.43f, 1.0f }
            );

            outlineColor = UIStateColor(
                { 0.6f, 0.6f, 0.6f, 1.0f },
                { 0.7f, 0.7f, 0.7f, 1.0f }
            );
        }

        UIButtonDecorate Fill(const glm::vec4 &color)
        {
            fillColor = UIStateColor(color);
            return *this;
        }

        UIButtonDecorate Fill(const glm::vec4 &normal, const glm::vec4 &hover)
        {
            fillColor = UIStateColor(normal, hover);
            return *this;
        }

        UIButtonDecorate Outline(const glm::vec4 &color)
        {
            outlineColor = UIStateColor(color);
            return *this;
        }

        UIButtonDecorate Outline(const glm::vec4 &normal, const glm::vec4 &hover)
        {
            outlineColor = UIStateColor(normal, hover);
            return *this;
        }
    };

    struct UIButton
    {
        std::string text;
        UIButtonDecorate decorate;
        
        // only for getter
        std::function<void()> onClick;

        bool isHovered = false;
        bool isClicked = false;

        UIButton(const std::string &text, const UIButtonDecorate &decorate, const std::function<void()> &onClick)
            : text(text), decorate(decorate), onClick(onClick)
        {
        }
    };
}