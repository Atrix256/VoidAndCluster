#include "image.h"
#include <algorithm>

void AppendImageHorizontal(
    const std::vector<uint8_t>& srcA,
    size_t widthA,
    size_t heightA,
    const std::vector<uint8_t>& srcB,
    size_t widthB,
    size_t heightB,
    std::vector<uint8_t>& dest,
    size_t& destWidth,
    size_t& destHeight
)
{
    // calculate dims of desination and allocate it, filled with 0s
    destWidth = widthA + widthB;
    destHeight = std::max(heightA, heightB);
    dest.resize(destWidth*destHeight, 0);

    // copy srcA in
    for (size_t y = 0; y < heightA; ++y)
    {
        const uint8_t* srcptr = &srcA[y*widthA];
        uint8_t* destptr = &dest[y*destWidth];

        memcpy(destptr, srcptr, widthA);
    }

    // copy srcB in
    for (size_t y = 0; y < heightB; ++y)
    {
        const uint8_t* srcptr = &srcB[y*widthB];
        uint8_t* destptr = &dest[y*destWidth + widthA];

        memcpy(destptr, srcptr, widthA);
    }
}