#pragma once

#include <vector>

// http://cv.ulichney.com/papers/1993-void-cluster.pdf
void GenerateBN_Void_Cluster(std::vector<uint8_t>& blueNoise, size_t width, bool useMitchellsBestCandidate, const char* baseFileName);
