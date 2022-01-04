#pragma once

#include <chrono>

class Timer
{
public:
    Timer()
    {
        reset();
    }
    double delta() const
    {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_lastTimestamp);
        return static_cast<double>(duration.count()) / 1000.0 / 1000.0;
    }
    void reset()
    {
        m_lastTimestamp = std::chrono::high_resolution_clock::now();
    }
private:
    using TimeStamp = std::chrono::time_point<std::chrono::high_resolution_clock>;
    TimeStamp m_lastTimestamp;
};