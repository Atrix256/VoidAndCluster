#pragma once

#include <vector>

enum class InitialPointSetModes
{
	Standard,    // Initial binary pattern and phase 1, as described in the paper
	MWCOrdering, // Initial binary pattern as described in the paper. Phase 1 uses "Mitchell's Worst Candidate" to get ordering.
	MBC          // Initial binary pattern and phase 1 replaced by "Mitchell's Best Candidtae"
};

inline const char* ToString(InitialPointSetModes mode)
{
	switch (mode)
	{
		case InitialPointSetModes::Standard: return "Standard";
		case InitialPointSetModes::MWCOrdering: return "MWCOrdering";
		case InitialPointSetModes::MBC: return "MBC";
	}
	return "Unknown";
}

// http://cv.ulichney.com/papers/1993-void-cluster.pdf
void GenerateBN_Void_Cluster(std::vector<uint8_t>& blueNoise, size_t width, float sigma, InitialPointSetModes initialPointSetMode, const char* baseFileName);
