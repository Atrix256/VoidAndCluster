#pragma once

#include "simple_fft/fft_settings.h"
#include "simple_fft/fft.h"

#include "misc.h"

#include <algorithm>
#include <vector>

struct ComplexImage2D
{
    ComplexImage2D(size_t w, size_t h)
    {
        m_width = w;
        m_height = h;
        pixels.resize(w*h, real_type(0.0f));
    }

    size_t m_width;
    size_t m_height;
    std::vector<complex_type> pixels;

    complex_type& operator()(size_t x, size_t y)
    {
        return pixels[y*m_width + x];
    }

    const complex_type& operator()(size_t x, size_t y) const
    {
        return pixels[y*m_width + x];
    }
};

template <typename T>
void DFT(const std::vector<T>& imageSrc, std::vector<T>& imageDest, size_t width)
{
    // convert the source image to float and store it in a complex image so it can be DFTd
    ComplexImage2D complexImageIn(width, width);
    for (size_t index = 0, count = width * width; index < count; ++index)
        complexImageIn.pixels[index] = float(imageSrc[index]) / float(std::numeric_limits<T>::max());

    // DFT the image to get frequency of the samples
    const char* error = nullptr;
    ComplexImage2D complexImageOut(width, width);
    simple_fft::FFT(complexImageIn, complexImageOut, width, width, error);

    // TODO: for some reason, the DC is huge. i'm not sure why...
    complexImageOut(0, 0) = 0.0f;

    // get the magnitudes and max magnitude
    std::vector<float> magnitudes;
    float maxMag = 0.0f;
    {
        magnitudes.resize(width * width, 0.0f);
        float* dest = magnitudes.data();
        for (size_t y = 0; y < width; ++y)
        {
            size_t srcY = (y + width / 2) % width;
            for (size_t x = 0; x < width; ++x)
            {
                size_t srcX = (x + width / 2) % width;

                const complex_type& c = complexImageOut(srcX, srcY);
                float mag = float(sqrt(c.real()*c.real() + c.imag()*c.imag()));
                maxMag = std::max(mag, maxMag);
                *dest = mag;
                ++dest;
            }
        }
    }

    // normalize the magnitudes and convert it back to a type T image
    //const float c = 1.0f / log(1.0f / 255.0f + maxMag);
    {
        imageDest.resize(width * width);
        const float* src = magnitudes.data();
        T* dest = imageDest.data();
        for (size_t y = 0; y < width; ++y)
        {
            for (size_t x = 0; x < width; ++x)
            {
                //float normalized = c * log(1.0f / 255.0f + *src);
                float normalized = *src / maxMag;

                float value = Lerp(0, float(std::numeric_limits<T>::max() + 1), normalized);
                value = Clamp(0.0f, float(std::numeric_limits<T>::max()), value);
                *dest = T(value);

                ++src;
                ++dest;
            }
        }
    }
}