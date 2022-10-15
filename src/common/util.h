#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

std::vector<uint8_t> loadFile(const char* filename);

template<size_t N, typename T>
constexpr auto lengthof(const T (&)[N])
{
  return N;
}
