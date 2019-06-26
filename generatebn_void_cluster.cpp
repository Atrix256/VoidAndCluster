#define _CRT_SECURE_NO_WARNINGS

#include "generatebn_void_cluster.h"
#include "whitenoise.h"
#include "convert.h"
#include "stb/stb_image_write.h"
#include "scoped_timer.h"

static const float c_sigma = 1.9f;// 1.5f;
static const float c_2sigmaSquared = 2.0f * c_sigma * c_sigma;
static const int c_3sigmaint = int(ceil(c_sigma * 3.0f));

static void SaveLUTImage(const std::vector<bool>& binaryPattern, std::vector<float>& LUT, size_t width, const char* fileName)
{
    // get the LUT min and max
    float LUTMin = LUT[0];
    float LUTMax = LUT[0];
    for (float f : LUT)
    {
        LUTMin = std::min(LUTMin, f);
        LUTMax = std::max(LUTMax, f);
    }

    size_t c_scale = 4;

    std::vector<uint8_t> image(width*width * c_scale*c_scale * 3);
    for (size_t index = 0; index < width*width*c_scale*c_scale; ++index)
    {
        size_t x = (index % (width * c_scale)) / c_scale;
        size_t y = index / (width * c_scale * c_scale);

        float percent = (LUT[y*width + x] - LUTMin) / (LUTMax - LUTMin);
        uint8_t value = FromFloat<uint8_t>(percent);

        image[index * 3 + 0] = value;
        image[index * 3 + 1] = value;
        image[index * 3 + 2] = value;

        if (binaryPattern[y*width + x])
        {
            image[index * 3 + 0] = 0;
            image[index * 3 + 1] = 255;
            image[index * 3 + 2] = 0;
        }
    }
    stbi_write_png(fileName, int(width*c_scale), int(width*c_scale), 3, image.data(), 0);
}

#if 1

template <bool CLUSTER>
static bool FindWinnerLUT(const std::vector<float>& LUT, const std::vector<bool>& binaryPattern, size_t width, int &bestPixelX, int& bestPixelY, std::mt19937& rng)
{
    float bestValue = CLUSTER ? -FLT_MAX : FLT_MAX;
    std::vector<size_t> bestIndices;
    for (size_t index = 0, count = LUT.size(); index < count; ++index)
    {
        if (binaryPattern[index] == CLUSTER)
        {
            if (LUT[index] == bestValue)
            {
                bestIndices.push_back(index);
            }
            else if ((CLUSTER == true && LUT[index] > bestValue) || (CLUSTER == false && LUT[index] < bestValue))
            {
                bestValue = LUT[index];
                bestIndices.clear();
                bestIndices.push_back(index);
            }
        }
    }

    if (bestIndices.size() == 0)
        return false;

    size_t bestIndex = bestIndices[0];

    // can randomize the winners
    /*
    if (bestIndices.size() > 1)
    {
        std::uniform_int_distribution<size_t> dist(0, bestIndices.size() - 1);
        bestIndex = bestIndices[dist(rng)];
    }
    */

    bestPixelX = int(bestIndex % width);
    bestPixelY = int(bestIndex / width);

    return true;
}

static bool FindTightestClusterLUT(const std::vector<float>& LUT, const std::vector<bool>& binaryPattern, size_t width, int &bestPixelX, int& bestPixelY, std::mt19937& rng)
{
    return FindWinnerLUT<true>(LUT, binaryPattern, width, bestPixelX, bestPixelY, rng);
}

static bool FindLargestVoidLUT(const std::vector<float>& LUT, const std::vector<bool>& binaryPattern, size_t width, int &bestPixelX, int& bestPixelY, std::mt19937& rng)
{
    return FindWinnerLUT<false>(LUT, binaryPattern, width, bestPixelX, bestPixelY, rng);
}

#else
static bool FindTightestClusterLUT(const std::vector<float>& LUT, const std::vector<bool>& binaryPattern, size_t width, int &bestPixelX, int& bestPixelY, std::mt19937& rn)
{
    float bestValue = -FLT_MAX;
    size_t bestIndex = ~size_t(0);
    for (size_t index = 0, count = LUT.size(); index < count; ++index)
    {
        if (binaryPattern[index] && LUT[index] > bestValue)
        {
            bestValue = LUT[index];
            bestIndex = index;
        }
    }

    if (bestIndex == ~size_t(0))
        return false;

    bestPixelX = int(bestIndex % width);
    bestPixelY = int(bestIndex / width);

    return true;
}

static bool FindLargestVoidLUT(const std::vector<float>& LUT, const std::vector<bool>& binaryPattern, size_t width, int &bestPixelX, int& bestPixelY, std::mt19937& rn)
{
    float bestValue = FLT_MAX;
    size_t bestIndex = ~size_t(0);
    for (size_t index = 0, count = LUT.size(); index < count; ++index)
    {
        if (!binaryPattern[index] && LUT[index] < bestValue)
        {
            bestValue = LUT[index];
            bestIndex = index;
        }
    }

    if (bestIndex == ~size_t(0))
        return false;

    bestPixelX = int(bestIndex % width);
    bestPixelY = int(bestIndex / width);

    return true;
}
#endif

static void WriteLUTValue(std::vector<float>& LUT, size_t width, bool value, int basex, int basey)
{
    #pragma omp parallel for
    for (int y = 0; y < width; ++y)
    {
        float disty = abs(float(y) - float(basey));
        if (disty > float(width / 2))
            disty = float(width) - disty;

        for (size_t x = 0; x < width; ++x)
        {
            float distx = abs(float(x) - float(basex));
            if (distx > float(width / 2))
                distx = float(width) - distx;

            float distanceSquared = float(distx*distx) + float(disty*disty);
            float energy = exp(-distanceSquared / c_2sigmaSquared) * (value ? 1.0f : -1.0f);
            LUT[y*width + x] += energy;
        }
    }
}

static void MakeLUT(const std::vector<bool>& binaryPattern, std::vector<float>& LUT, size_t width, bool writeOnes)
{
    LUT.clear();
    LUT.resize(width*width, 0.0f);
    for (size_t index = 0; index < width*width; ++index)
    {
        if (binaryPattern[index] == writeOnes)
        {
            int x = int(index % width);
            int y = int(index / width);
            WriteLUTValue(LUT, width, writeOnes, x, y);
        }
    }
}

#if SAVE_VOIDCLUSTER_INITIALBP()

static void SaveBinaryPattern(const std::vector<bool>& binaryPattern, size_t width, const char* baseFileName, int iterationCount, int tightestClusterX, int tightestClusterY, int largestVoidX, int largestVoidY)
{
    size_t c_scale = 4;

    std::vector<uint8_t> binaryPatternImage(width*width * c_scale*c_scale * 3);
    for (size_t index = 0; index < width*width*c_scale*c_scale; ++index)
    {
        size_t x = (index % (width * c_scale)) / c_scale;
        size_t y = index / (width * c_scale * c_scale);

        bool isCluster = (x == tightestClusterX && y == tightestClusterY);
        bool isVoid = (x == largestVoidX && y == largestVoidY);

        if (isCluster == isVoid)
        {
            if (isCluster)
            {
                binaryPatternImage[index * 3 + 0] = 255;
                binaryPatternImage[index * 3 + 1] = 255;
                binaryPatternImage[index * 3 + 2] = 0;
            }
            else
            {
                binaryPatternImage[index * 3 + 0] = binaryPattern[y*width+x] ? 255 : 0;
                binaryPatternImage[index * 3 + 1] = binaryPattern[y*width + x] ? 255 : 0;
                binaryPatternImage[index * 3 + 2] = binaryPattern[y*width + x] ? 255 : 0;
            }
        }
        else if (isCluster)
        {
            binaryPatternImage[index * 3 + 0] = 255;
            binaryPatternImage[index * 3 + 1] = 0;
            binaryPatternImage[index * 3 + 2] = 0;
        }
        else if (isVoid)
        {
            binaryPatternImage[index * 3 + 0] = 0;
            binaryPatternImage[index * 3 + 1] = 255;
            binaryPatternImage[index * 3 + 2] = 0;
        }
    }

    char fileName[256];
    sprintf(fileName, "%s%i.png", baseFileName, iterationCount);
    stbi_write_png(fileName, int(width*c_scale), int(width*c_scale), 3, binaryPatternImage.data(), 0);
}

#endif

static void MakeInitialBinaryPattern(std::vector<bool>& binaryPattern, size_t width, const char* baseFileName, std::mt19937& rng)
{
    ScopedTimer timer("Initial Pattern", false);

    std::uniform_int_distribution<size_t> dist(0, width*width);

    std::vector<float> LUT;
    LUT.resize(width*width, 0.0f);

    binaryPattern.resize(width*width, false);
    size_t ones = size_t(float(width*width) * 0.1f); // start 10% of the pixels as white
    for (size_t index = 0; index < ones; ++index)
    {
        size_t pixel = dist(rng);
        binaryPattern[pixel] = true;
        WriteLUTValue(LUT, width, true, int(pixel % width), int(pixel / width));
    }

    int iterationCount = 0;
    while (1)
    {
        printf("\r%i iterations", iterationCount);
        iterationCount++;

        // find the location of the tightest cluster
        int tightestClusterX = -1;
        int tightestClusterY = -1;
        FindTightestClusterLUT(LUT, binaryPattern, width, tightestClusterX, tightestClusterY, rng);

        // remove the 1 from the tightest cluster
        binaryPattern[tightestClusterY*width + tightestClusterX] = false;
        WriteLUTValue(LUT, width, false, tightestClusterX, tightestClusterY);

        // find the largest void
        int largestVoidX = -1;
        int largestVoidY = -1;
        FindLargestVoidLUT(LUT, binaryPattern, width, largestVoidX, largestVoidY, rng);

        // put the 1 in the largest void
        binaryPattern[largestVoidY*width + largestVoidX] = true;
        WriteLUTValue(LUT, width, true, largestVoidX, largestVoidY);

        #if SAVE_VOIDCLUSTER_INITIALBP()
        // save the binary pattern out for debug purposes
        SaveBinaryPattern(binaryPattern, width, baseFileName, iterationCount, tightestClusterX, tightestClusterY, largestVoidX, largestVoidY);
        #endif

        // exit condition. the pattern is stable
        if (tightestClusterX == largestVoidX && tightestClusterY == largestVoidY)
            break;
    }
    printf("\n");
}

// Phase 1: Start with initial binary pattern and remove the tightest cluster until there are none left, entering ranks for those pixels
static void Phase1(std::vector<bool>& binaryPattern, std::vector<float>& LUT, std::vector<size_t>& ranks, size_t width, std::mt19937& rng, const char* baseFileName)
{
    ScopedTimer timer("Phase 1", false);

    // count how many ones there are
    size_t ones = 0;
    for (bool b : binaryPattern)
    {
        if (b)
            ones++;
    }
    size_t startingOnes = ones;

    // remove the tightest cluster repeatedly
    while (ones > 0)
    {
        printf("\r%i%%", int(100.0f * (1.0f - float(ones) / float(startingOnes))));

        int bestX, bestY;
        FindTightestClusterLUT(LUT, binaryPattern, width, bestX, bestY, rng);
        binaryPattern[bestY * width + bestX] = false;
        WriteLUTValue(LUT, width, false, bestX, bestY);
        ones--;
        ranks[bestY*width + bestX] = ones;

        #if SAVE_VOIDCLUSTER_PHASE1()
        // save the binary pattern out for debug purposes
        SaveBinaryPattern(binaryPattern, width, baseFileName, int(startingOnes - ones), bestX, bestY, -1, -1);
        #endif
    }
    printf("\n");
}

struct Point
{
    size_t x;
    size_t y;
};
typedef std::vector<Point> TPoints;
typedef std::vector<TPoints> TPointGrid;

static bool DistanceSqToClosestPoint(const TPoints& points, const Point& point, float& minDistSq, size_t width)
{
    if (points.size() == 0)
        return false;

    // calculate the closest distance from this point to an existing sample
    for (const Point& p : points)
    {
        float distx = std::abs(float(p.x) - float(point.x));
        float disty = std::abs(float(p.y) - float(point.y));

        if (distx > float(width) / 2.0f)
            distx = float(width) - distx;

        if (disty > float(width) / 2.0f)
            disty = float(width) - disty;

        float distSq = distx * distx + disty * disty;
        if (distSq < minDistSq)
            minDistSq = distSq;
    }
    return true;
}

static float DistanceSqToClosestPoint(const TPointGrid& grid, size_t cellCount, size_t cellSize, const Point& point, size_t width)
{
    const int basex = int(point.x / cellSize);
    const int basey = int(point.y / cellSize);

    const int maxRadius = int(cellCount / 2);

    float minDistSq = FLT_MAX;
    bool foundAPoint = false;
    bool didAnExtraRing = false;

    for (int radius = 0; radius <= maxRadius; ++radius)
    {
        // top and bottom rows
        {
            for (int offsetX = -radius; offsetX <= radius; ++offsetX)
            {
                int x = int(basex + offsetX + cellCount) % int(cellCount);

                int offsetY = -radius;
                int y = int(basey + offsetY + cellCount) % int(cellCount);
                foundAPoint |= DistanceSqToClosestPoint(grid[y*cellCount + x], point, minDistSq, width);

                offsetY = radius;
                y = int(basey + offsetY + cellCount) % int(cellCount);
                foundAPoint |= DistanceSqToClosestPoint(grid[y*cellCount + x], point, minDistSq, width);
            }
        }

        // left and right
        {
            for (int offsetY = -radius + 1; offsetY <= radius - 1; ++offsetY)
            {
                int y = int(basey + offsetY + cellCount) % int(cellCount);

                int offsetX = -radius;
                int x = int(basex + offsetX + cellCount) % int(cellCount);
                foundAPoint |= DistanceSqToClosestPoint(grid[y*cellCount + x], point, minDistSq, width);

                offsetX = +radius;
                x = int(basex + offsetX + cellCount) % int(cellCount);
                foundAPoint |= DistanceSqToClosestPoint(grid[y*cellCount + x], point, minDistSq, width);
            }
        }

        // we stop when we've found a point, then do another ring to make sure there isn't something closer to what we found.
        if (foundAPoint)
        {
            if (didAnExtraRing)
                break;
            else
                didAnExtraRing = true;
        }
    }

    return minDistSq;
}

static void AddPointToPointGrid(TPointGrid& grid, size_t cellCount, size_t cellSize, const Point& point)
{
    Point cell;
    cell.x = point.x / cellSize;
    cell.y = point.y / cellSize;
    grid[cell.y * cellCount + cell.x].push_back(point);
}

// This replaces "Initial Binary Pattern" and "Phase 1" in the void and cluster algorithm.
// Initial binary pattern makes blue noise distributed points.
// Phase 1 makes them be progressive, so any points from 0 to N are blue noise.
// Mitchell's best candidate algorithm makes progressive blue noise so can be used instead of those 2 steps.
// https://blog.demofox.org/2017/10/20/generating-blue-noise-sample-points-with-mitchells-best-candidate-algorithm/
static void MitchellsBestCandidate(std::vector<bool>& binaryPattern, std::vector<size_t>& ranks, size_t width)
{
    ScopedTimer timer("Mitchells Best Candidate", false);

    std::mt19937 rng(GetRNGSeed());
    std::uniform_int_distribution<size_t> dist(0, width*width - 1);

    binaryPattern.resize(width*width, false);
    ranks.resize(width*width, ~size_t(0));

    static const size_t gridCellCount = 32;
    TPointGrid grid(gridCellCount*gridCellCount);
    const size_t gridCellSize = width / gridCellCount;

    size_t ones = size_t(float(width * width)*0.1f);
    for (size_t i = 0; i < ones; ++i)
    {
        printf("\r%i%%", int(100.0f * float(i) / float(ones - 1)));

        // we scale up the candidates each iteration like in the paper, to keep frequency behavior consistent
        size_t numCandidates = i + 1;

        // keep the candidate that is farthest from the closest existing point
        float bestDistanceSq = 0.0f;
        Point best;
        for (size_t candidate = 0; candidate < numCandidates; ++candidate)
        {
            size_t index = dist(rng);
            Point c;
            c.x = index % width;
            c.y = index / width;

            float minDistSq = DistanceSqToClosestPoint(grid, gridCellCount, gridCellSize, c, width);

            if (minDistSq > bestDistanceSq)
            {
                bestDistanceSq = minDistSq;
                best = c;
            }
        }

        // take the best candidate
        binaryPattern[best.y * width + best.x] = true;
        ranks[best.y * width + best.x] = i;
        AddPointToPointGrid(grid, gridCellCount, gridCellSize, best);
    }
    printf("\n");
}

// Phase 2: Start with initial binary pattern and add points to the largest void until half the pixels are white, entering ranks for those pixels
static void Phase2(std::vector<bool>& binaryPattern, std::vector<float>& LUT, std::vector<size_t>& ranks, size_t width, std::mt19937& rng)
{
    ScopedTimer timer("Phase 2", false);

    // count how many ones there are
    size_t ones = 0;
    for (bool b : binaryPattern)
    {
        if (b)
            ones++;
    }
    size_t startingOnes = ones;
    size_t onesToDo = (width*width / 2) - startingOnes;

    // add to the largest void repeatedly
    while (ones <= (width*width/2))
    {
        size_t onesDone = ones - startingOnes;
        printf("\r%i%%", int(100.0f * float(onesDone) / float(onesToDo)));

        int bestX, bestY;
        FindLargestVoidLUT(LUT, binaryPattern, width, bestX, bestY, rng);
        binaryPattern[bestY * width + bestX] = true;
        WriteLUTValue(LUT, width, true, bestX, bestY);
        ranks[bestY*width + bestX] = ones;
        ones++;
    }
    printf("\n");
}

// Phase 3: Continue with the last binary pattern, repeatedly find the tightest cluster of 0s and insert a 1 into them
static void Phase3(std::vector<bool>& binaryPattern, std::vector<float>& LUT, std::vector<size_t>& ranks, size_t width, std::mt19937& rng)
{
    ScopedTimer timer("Phase 3", false);

    // count how many ones there are
    size_t ones = 0;
    for (bool b : binaryPattern)
    {
        if (b)
            ones++;
    }
    size_t startingOnes = ones;
    size_t onesToDo = (width*width) - startingOnes;

    // add 1 to the largest cluster of 0's repeatedly
    int bestX, bestY;
    while (FindLargestVoidLUT(LUT, binaryPattern, width, bestX, bestY, rng))
    {
        size_t onesDone = ones - startingOnes;
        printf("\r%i%%", int(100.0f * float(onesDone) / float(onesToDo)));

        WriteLUTValue(LUT, width, true, bestX, bestY);
        binaryPattern[bestY * width + bestX] = true;
        ranks[bestY*width + bestX] = ones;
        ones++;
    }
    printf("\n");
}

void GenerateBN_Void_Cluster(std::vector<uint8_t>& blueNoise, size_t width, bool useMitchellsBestCandidate, const char* baseFileName)
{
    std::mt19937 rng(GetRNGSeed());

    std::vector<size_t> ranks(width*width, ~size_t(0));

    std::vector<bool> initialBinaryPattern;
    std::vector<bool> binaryPattern;
    std::vector<float> initialLUT;
    std::vector<float> LUT;

    if (!useMitchellsBestCandidate)
    {
        // make the initial binary pattern and initial LUT
        MakeInitialBinaryPattern(initialBinaryPattern, width, baseFileName, rng);
        MakeLUT(initialBinaryPattern, initialLUT, width, true);

        // Phase 1: Start with initial binary pattern and remove the tightest cluster until there are none left, entering ranks for those pixels
        binaryPattern = initialBinaryPattern;
        LUT = initialLUT;
        Phase1(binaryPattern, LUT, ranks, width, rng, baseFileName);
    }
    else
    {
        // replace initial binary pattern and phase 1 with Mitchell's best candidate algorithm, and then making the LUT
        MitchellsBestCandidate(initialBinaryPattern, ranks, width);
        MakeLUT(initialBinaryPattern, initialLUT, width, true);

        //SaveBinaryPattern(initialBinaryPattern, width, "out/_blah", 0, -1, -1, -1, -1);
    }

    // Phase 2: Start with initial binary pattern and add points to the largest void until half the pixels are white, entering ranks for those pixels
    binaryPattern = initialBinaryPattern;
    LUT = initialLUT;
    Phase2(binaryPattern, LUT, ranks, width, rng);

    // Phase 3: Continue with the last binary pattern, repeatedly find the tightest cluster of 0s and insert a 1 into them
    // Note: we do need to re-make the LUT, because we are writing 0s instead of 1s
    MakeLUT(binaryPattern, LUT, width, false);
    Phase3(binaryPattern, LUT, ranks, width, rng);

    // convert to U8
    {
        ScopedTimer timer("Converting to U8", false);
        blueNoise.resize(width*width);
        for (size_t index = 0; index < width*width; ++index)
            blueNoise[index] = uint8_t(ranks[index] * 256 / (width*width));
    }
}
