#pragma once

template <typename T>
T Clamp(T min, T max, T value)
{
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}

inline float Lerp(float a, float b, float t)
{
    return a * (1.0f - t) + b * t;
}