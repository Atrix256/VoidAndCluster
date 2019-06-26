#pragma once

#include <vector>

template<typename T>
void WriteHistogram(const std::vector<T>& values, const char* fileName)
{
    std::vector<size_t> histogram;
    histogram.resize(std::numeric_limits<T>::max() + 1, 0);
    for (const T& value : values)
        histogram[value]++;

    FILE * file = nullptr;
    fopen_s(&file, fileName, "w+t");
    fprintf(file, "\"Value\",\"Count\"\n");
    for (size_t index = 0, count = histogram.size(); index < count; ++index)
        fprintf(file, "\"%zu\",\"%zu\"\n", index, histogram[index]);
    fclose(file);
}