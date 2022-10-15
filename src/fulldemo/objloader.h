#pragma once

#include <vector>

struct Vertex
{
  float x, y, z;
  float nx, ny, nz;
};

struct Color
{
  float r, g, b;
};

struct Material
{
  Color ambient;
  Color diffuse;
  Color emissive;
};

// A mesh with only one material
struct PlainMesh
{
  int material;
  std::vector<Vertex> vertices;
};

struct Scene
{
  std::vector<Material> materials;
  std::vector<PlainMesh> plainMeshes;
};

Scene loadObj(const char* path);
