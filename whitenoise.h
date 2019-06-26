#pragma once

#include <random>
#include "misc.h"
#include "settings.h"

inline std::seed_seq& GetRNGSeed()
{
#if DETERMINISTIC()
    static std::seed_seq fullSeed{ DETERMINISTIC_SEED() };
#else
    static std::random_device rd;
    static std::seed_seq fullSeed{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
#endif
    return fullSeed;
}

template <typename T>
inline T RandomValue(std::mt19937& rng)
{
    std::uniform_int_distribution<T> dist(0, std::numeric_limits<T>::max());
    return dist(rng);
}

template <>
inline uint8_t RandomValue<uint8_t>(std::mt19937& rng)
{
    std::uniform_int_distribution<uint16_t> dist(0, 255);  // can't be uint8_t :/
    return uint8_t(dist(rng));
}

template <typename T>
inline void MakeWhiteNoise(std::mt19937& rng, std::vector<T>& pixels, size_t width)
{
    pixels.resize(width*width);

    // NOTE: this works, but won't give a balanced histogram
    //for (T& pixel : pixels)
        //pixel = RandomValue<T>();

    for (size_t index = 0, count = width * width; index < count; ++index)
    {
        float percent = float(index) / float(count - 1);
        float value = Lerp(0, float(std::numeric_limits<T>::max() + 1), percent); // intentionally not using convert.h conversion. this is subtly different.
        value = Clamp(0.0f, float(std::numeric_limits<T>::max()), value);
        pixels[index] = T(value);
    }

    std::shuffle(pixels.begin(), pixels.end(), rng);
}

inline void MakeWhiteNoiseFloat(std::mt19937& rng, std::vector<float>& pixels, size_t width)
{
    pixels.resize(width*width);

    for (size_t index = 0, count = width * width; index < count; ++index)
        pixels[index] = float(index) / float(count - 1);

    std::shuffle(pixels.begin(), pixels.end(), rng);
}