#pragma once

#include <glm/glm.hpp>

namespace ignite {

    struct DirLight
    {
        glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // RGBA
        glm::vec4 direction = { 0.0f, 1.0f, 0.0f, 0.0f }; // Normalized direction
        float intensity = 0.5f;           // Brightness scalar
        float angularSize = 0.1f;         // Simulates soft shadows (sun size in degrees)
        float ambientIntensity = 0.1f;    // Ambient light component
        float shadowStrength = 1.0f;      // Strength of cast shadows (0 = no shadow, 1 = full)
        float pad[1];

        DirLight() { ++count; }
        ~DirLight() { --count; }

        static uint32_t count;
    };
}
