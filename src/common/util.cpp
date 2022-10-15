#include "util.h"

#include <cstdio>
#include <stdexcept>

std::vector<uint8_t> loadFile(const char* filename)
{
  FILE* fp = fopen(filename, "rb");

  if(!fp)
  {
    char buffer[256];
    snprintf(buffer, sizeof buffer, "failed to open file '%s'", filename);
    throw std::runtime_error(buffer);
  }

  fseek(fp, 0, SEEK_END);
  const auto fileSize = ftell(fp);
  std::vector<uint8_t> buffer(fileSize);

  fseek(fp, 0, SEEK_SET);
  fread(buffer.data(), 1, fileSize, fp);

  fclose(fp);

  return buffer;
}
