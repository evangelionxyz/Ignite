#pragma once

#include <stdint.h>
#include <spdlog/fmt/fmt.h>

namespace ignite::joystick{

typedef uint32_t JState;
typedef uint8_t JButton;

    namespace state {
        enum : JState
        {
            CONNECTED    = 0x00040001,
            DISCONNECTED = 0x00040002ED
        };
    }

    namespace button {
        enum : JButton
        {
            A             = 0,
            B             = 1,
            X             = 2,
            Y             = 3,
            LEFT_BUMPER   = 4,
            RIGHT_BUMPER  = 5,
            BACK          = 6,
            START         = 7,
            GUIDE         = 8,
            LEFT_THUMB    = 9,
            RIGHT_THUMB   = 10,
            DPAD_UP       = 11,
            DPAD_RIGHT    = 12,
            DPAD_DOWN     = 13,
            DPAD_LEFT     = 14,
            LAST          = DPAD_LEFT,
            CROSS         = A,
            CIRCLE        = B,
            SQUARE        = X,
            TRIANGLE      = Y
        };
    }

    namespace axis {
        enum JAxis
        {
            LEFT_X        = 0,
            LEFT_Y        = 1,
            RIGHT_X       = 2,
            RIGHT_Y       = 3,
            LEFT_TRIGGER  = 4,
            RIGHT_TRIGGER = 5,
            LAST          = RIGHT_TRIGGER
        };
    }

    static std::string ButtonToString(JButton bt)
    {
        switch (bt)
        {
            case button::A: return "A";
            case button::B: return "B";
            case button::X: return "X";
            case button::Y: return "Y";
            case button::LEFT_BUMPER: return "LB";
            case button::RIGHT_BUMPER: return "RB";
            case button::BACK: return "Back";
            case button::START: return "Start";
            case button::GUIDE: return "Guide";
            case button::LEFT_THUMB: return "LThumb";
            case button::RIGHT_THUMB: return "RThumb";
            case button::DPAD_UP: return "DPadUp";
            case button::DPAD_RIGHT: return "DPadRight";
            case button::DPAD_DOWN: return "DPadDown";
            case button::DPAD_LEFT: return "DPadLeft";
            default: return fmt::format("Unknown ({})", bt);
        }
    }
}
