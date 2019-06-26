#define _CRT_SECURE_NO_WARNINGS

#define THRESHOLD_SAMPLES() 11 // the number of samples for threshold testing.

#include <vector>
#include <stdint.h>

#include "convert.h"
#include "histogram.h"
#include "image.h"
#include "scoped_timer.h"
#include "generatebn_void_cluster.h"
#include "dft.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

void TestMask(const std::vector<uint8_t>& noise, size_t noiseSize, const char* baseFileName)
{
    std::vector<uint8_t> thresholdImage(noise.size());

    for (size_t testIndex = 0; testIndex < THRESHOLD_SAMPLES(); ++testIndex)
    {
        float percent = float(testIndex) / float(THRESHOLD_SAMPLES() - 1);
        uint8_t thresholdValue = FromFloat<uint8_t>(percent);
        if (thresholdValue == 0)
            thresholdValue = 1;
        else if (thresholdValue == 255)
            thresholdValue = 254;

        for (size_t pixelIndex = 0, pixelCount = noise.size(); pixelIndex < pixelCount; ++pixelIndex)
            thresholdImage[pixelIndex] = noise[pixelIndex] > thresholdValue ? 255 : 0;

        std::vector<uint8_t> thresholdImageDFT;
        DFT(thresholdImage, thresholdImageDFT, noiseSize);

        std::vector<uint8_t> noiseAndDFT;
        size_t noiseAndDFT_width = 0;
        size_t noiseAndDFT_height = 0;
        AppendImageHorizontal(thresholdImage, noiseSize, noiseSize, thresholdImageDFT, noiseSize, noiseSize, noiseAndDFT, noiseAndDFT_width, noiseAndDFT_height);

        char fileName[256];
        sprintf(fileName, "%s_%u.png", baseFileName, thresholdValue);
        stbi_write_png(fileName, int(noiseAndDFT_width), int(noiseAndDFT_height), 1, noiseAndDFT.data(), 0);
    }
}

void TestNoise(const std::vector<uint8_t>& noise, size_t noiseSize, const char* baseFileName)
{
    char fileName[256];
    sprintf(fileName, "%s.histogram.csv", baseFileName);

    WriteHistogram(noise, fileName);
    std::vector<uint8_t> noiseDFT;
    DFT(noise, noiseDFT, noiseSize);

    std::vector<uint8_t> noiseAndDFT;
    size_t noiseAndDFT_width = 0;
    size_t noiseAndDFT_height = 0;
    AppendImageHorizontal(noise, noiseSize, noiseSize, noiseDFT, noiseSize, noiseSize, noiseAndDFT, noiseAndDFT_width, noiseAndDFT_height);

    sprintf(fileName, "%s.png", baseFileName);
    stbi_write_png(fileName, int(noiseAndDFT_width), int(noiseAndDFT_height), 1, noiseAndDFT.data(), 0);

    TestMask(noise, noiseSize, baseFileName);
}

int main(int argc, char** argv)
{
    // generate blue noise using void and cluster
    {
        static size_t c_width = 256;

        std::vector<uint8_t> noise;

        {
            ScopedTimer timer("Blue noise by void and cluster");
            GenerateBN_Void_Cluster(noise, c_width, false, "out/blueVC_1");
        }

        TestNoise(noise, c_width, "out/blueVC_1");
    }

    // generate blue noise using void and cluster but using mitchell's best candidate instead of initial binary pattern and phase 1
    {
        static size_t c_width = 256;

        std::vector<uint8_t> noise;

        {
            ScopedTimer timer("Blue noise by void and cluster with Mitchells best candidate");
            GenerateBN_Void_Cluster(noise, c_width, true, "out/blueVC_1M");
        }

        TestNoise(noise, c_width, "out/blueVC_1M");
    }

    // load a blue noise texture
    {
        int width, height, channels;

        std::vector<uint8_t> noise;

        {
            ScopedTimer timer("Blue noise by void and cluster from loading the texture");
            uint8_t* image = stbi_load("bluenoise256.png", &width, &height, &channels, 0);

            noise.reserve(width*height);
            for (int i = 0; i < width*height; ++i)
                noise.push_back(image[i*channels]);

            stbi_image_free(image);
        }

        TestNoise(noise, width, "out/blueVC_2");
    }

    system("pause");

    return 0;
}