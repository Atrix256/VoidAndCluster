#pragma once

#include <vector>

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
);