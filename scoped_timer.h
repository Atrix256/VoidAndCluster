#pragma once

#include <chrono>

struct ScopedTimer
{
    ScopedTimer(const char* label, bool twoNewLines = true)
        : m_twoNewLines(twoNewLines)
    {
        m_start = std::chrono::high_resolution_clock::now();
        printf("%s...\n", label);
    }

    ~ScopedTimer()
    {
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end - m_start);
        printf("%f ms\n", time_span.count() * 1000.0f);
        if (m_twoNewLines)
            printf("\n");
    }

    bool m_twoNewLines;
    std::chrono::high_resolution_clock::time_point m_start;
};