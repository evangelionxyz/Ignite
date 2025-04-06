#pragma once
#include "types.hpp"

class Timestep
{
public:
    explicit Timestep(const f32 time = 0.0) : m_Time(time)
    {
    }

    explicit operator f32() const { return m_Time; }
    [[nodiscard]] f32 Seconds() const { return m_Time; }
    [[nodiscard]] f32 MilliSeconds() const { return m_Time * 1000.0f; }
private:
    f32 m_Time;
};

class Timer
{
public:
    Timer()
    {
        Reset();
    }

    void Reset()
    {
        m_Start = std::chrono::high_resolution_clock::now();
    }

    [[nodiscard]] f32 Elapsed() const
    {
        return static_cast<f32>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count()) * 0.001f * 0.001f * 0.001f;
    }

    [[nodiscard]] f32 ElapsedMillis() const
    {
        return Elapsed() * 1000.0f;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
};
