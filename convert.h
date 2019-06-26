#pragma once

#include <vector>

#include "misc.h"

template <typename T>
float ToFloat(T value)
{
    return float(value) / float(std::numeric_limits<T>::max());
}

template <typename T>
T FromFloat(float value)
{
    value = Lerp(0, float(std::numeric_limits<T>::max() + 1), value);
    value = Clamp(0.0f, float(std::numeric_limits<T>::max()), value);
    return T(value);
}

template <typename T>
void ToFloat(const std::vector<T>& src, std::vector<float>& dest)
{
    dest.resize(src.size());
    for (size_t index = 0, count = src.size(); index < count; ++index)
        dest[index] = ToFloat<T>(src[index]);
}

template <typename T>
void FromFloat(const std::vector<float>& src, std::vector<T>& dest)
{
    dest.resize(src.size());
    for (size_t index = 0, count = src.size(); index < count; ++index)
        dest[index] = FromFloat<T>(src[index]);
}