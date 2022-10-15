#include "objloader.h"

#include <cassert>
#include <cstdio>
#include <map>
#include <sstream>
#include <string>

namespace
{

struct Stream
{
  const char* ptr;
  int len;

  void operator+=(int n)
  {
    ptr += n;
    len -= n;
  }

  operator std::string() const { return std::string(ptr, len); }
};

Stream parseLine(Stream& input)
{
  Stream r{};
  r.ptr = input.ptr;

  while(input.len > 0 && input.ptr[0] != '\n')
    input += 1;

  r.len = input.ptr - r.ptr;

  // skip EOL
  input += 1;

  return r;
}

Stream parseWord(Stream& input)
{
  while(input.len > 0 && input.ptr[0] == ' ')
    input += 1;

  Stream r{};
  r.ptr = input.ptr;

  while(input.len > 0 && input.ptr[0] != ' ')
    input += 1;

  r.len = input.ptr - r.ptr;

  // skip separator
  input += 1;

  return r;
}

std::vector<char> loadFile(const char* path)
{
  FILE* fp = fopen(path, "rb");
  assert(fp);

  fseek(fp, 0, SEEK_END);
  const int size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  std::vector<char> r(size);
  fread(r.data(), 1, size, fp);

  fclose(fp);

  return r;
}

float parseFloat(Stream& line)
{
  auto word = parseWord(line);
  return strtof(word.ptr, nullptr);
}

Color parseColor(Stream& line)
{
  Color c;
  c.r = parseFloat(line);
  c.g = parseFloat(line);
  c.b = parseFloat(line);
  return c;
}

Material parseMaterial(Stream& s)
{
  Material result{};

  while(s.len > 0)
  {
    auto line = parseLine(s);
    if(line.len == 0)
      break;

    std::string cmd = parseWord(line);
    if(cmd == "Kd")
      result.diffuse = parseColor(line);
    else if(cmd == "Ka")
      result.ambient = parseColor(line);
    else if(cmd == "Ke")
      result.emissive = parseColor(line);
  }

  return result;
}

std::map<std::string, Material> loadMaterialLib(const char* path)
{
  std::map<std::string, Material> result;

  const auto contents = loadFile(path);

  Stream s{contents.data(), (int)contents.size()};

  while(s.len > 0)
  {
    auto line = parseLine(s);

    std::string cmd = parseWord(line);
    if(cmd == "newmtl")
    {
      std::string name = parseWord(line);
      result[name] = parseMaterial(s);
    }
  }

  return result;
}

[[maybe_unused]] Scene fakeLoadObj()
{
  // simulate parsing for now
  Scene r{};

  {
    r.materials.push_back({});
    Material& m = r.materials.back();
    m.diffuse = {0.4, 0.4, 0.3};
  }

  {
    r.materials.push_back({});
    Material& m = r.materials.back();
    m.diffuse = {0.7, 0.2, 0.1};
    m.emissive = {0.0, 0.5, 0.9};
  }

  // Floor
  {
    r.plainMeshes.push_back({});
    PlainMesh& mesh = r.plainMeshes.back();
    mesh.material = 0;
    mesh.vertices = {
          {/*pos*/ +8.0f, +8.0f, -2.0f, /*normal*/ 0, 0, 1},
          {/*pos*/ -8.0f, +8.0f, -2.0f, /*normal*/ 0, 0, 1},
          {/*pos*/ -8.0f, -8.0f, -2.0f, /*normal*/ 0, 0, 1},
          {/*pos*/ +8.0f, +8.0f, -2.0f, /*normal*/ 0, 0, 1},
          {/*pos*/ -8.0f, -8.0f, -2.0f, /*normal*/ 0, 0, 1},
          {/*pos*/ +8.0f, -8.0f, -2.0f, /*normal*/ 0, 0, 1},
    };
  }

  // Cube
  {
    r.plainMeshes.push_back({});
    PlainMesh& mesh = r.plainMeshes.back();
    mesh.material = 1;
    mesh.vertices = {
          {/*pos*/ -1.0f, -1.0f, -1.0f, /*normal*/ -1, 0, 0},
          {/*pos*/ -1.0f, -1.0f, +1.0f, /*normal*/ -1, 0, 0},
          {/*pos*/ -1.0f, +1.0f, +1.0f, /*normal*/ -1, 0, 0},

          {/*pos*/ +1.0f, +1.0f, -1.0f, /*normal*/ 0, 0, -1},
          {/*pos*/ -1.0f, -1.0f, -1.0f, /*normal*/ 0, 0, -1},
          {/*pos*/ -1.0f, +1.0f, -1.0f, /*normal*/ 0, 0, -1},

          {/*pos*/ +1.0f, -1.0f, +1.0f, /*normal*/ 0, -1, 0},
          {/*pos*/ -1.0f, -1.0f, -1.0f, /*normal*/ 0, -1, 0},
          {/*pos*/ +1.0f, -1.0f, -1.0f, /*normal*/ 0, -1, 0},

          {/*pos*/ +1.0f, +1.0f, -1.0f, /*normal*/ 0, 0, -1},
          {/*pos*/ +1.0f, -1.0f, -1.0f, /*normal*/ 0, 0, -1},
          {/*pos*/ -1.0f, -1.0f, -1.0f, /*normal*/ 0, 0, -1},

          {/*pos*/ -1.0f, -1.0f, -1.0f, /*normal*/ -1, 0, 0},
          {/*pos*/ -1.0f, +1.0f, +1.0f, /*normal*/ -1, 0, 0},
          {/*pos*/ -1.0f, +1.0f, -1.0f, /*normal*/ -1, 0, 0},

          {/*pos*/ +1.0f, -1.0f, +1.0f, /*normal*/ 0, -1, 0},
          {/*pos*/ -1.0f, -1.0f, +1.0f, /*normal*/ 0, -1, 0},
          {/*pos*/ -1.0f, -1.0f, -1.0f, /*normal*/ 0, -1, 0},

          {/*pos*/ -1.0f, +1.0f, +1.0f, /*normal*/ 0, 0, +1},
          {/*pos*/ -1.0f, -1.0f, +1.0f, /*normal*/ 0, 0, +1},
          {/*pos*/ +1.0f, -1.0f, +1.0f, /*normal*/ 0, 0, +1},

          {/*pos*/ +1.0f, +1.0f, +1.0f, /*normal*/ +1, 0, 0},
          {/*pos*/ +1.0f, -1.0f, -1.0f, /*normal*/ +1, 0, 0},
          {/*pos*/ +1.0f, +1.0f, -1.0f, /*normal*/ +1, 0, 0},

          {/*pos*/ +1.0f, -1.0f, -1.0f, /*normal*/ +1, 0, 0},
          {/*pos*/ +1.0f, +1.0f, +1.0f, /*normal*/ +1, 0, 0},
          {/*pos*/ +1.0f, -1.0f, +1.0f, /*normal*/ +1, 0, 0},

          {/*pos*/ +1.0f, +1.0f, +1.0f, /*normal*/ 0, +1, 0},
          {/*pos*/ +1.0f, +1.0f, -1.0f, /*normal*/ 0, +1, 0},
          {/*pos*/ -1.0f, +1.0f, -1.0f, /*normal*/ 0, +1, 0},

          {/*pos*/ +1.0f, +1.0f, +1.0f, /*normal*/ 0, +1, 0},
          {/*pos*/ -1.0f, +1.0f, -1.0f, /*normal*/ 0, +1, 0},
          {/*pos*/ -1.0f, +1.0f, +1.0f, /*normal*/ 0, +1, 0},

          {/*pos*/ +1.0f, +1.0f, +1.0f, /*normal*/ 0, 0, +1},
          {/*pos*/ -1.0f, +1.0f, +1.0f, /*normal*/ 0, 0, +1},
          {/*pos*/ +1.0f, -1.0f, +1.0f, /*normal*/ 0, 0, +1},
    };
  }

  return r;
}

} // namespace

Scene loadObj(const char* path)
{
  const auto contents = loadFile(path);

  struct float3
  {
    float x, y, z;
  };

  std::vector<float3> coords;
  std::vector<float3> normals;
  std::string currMaterial;

  Scene result{};

  std::vector<Vertex> faces;

  std::map<std::string, int> materialNameToId;

  auto flushFaces = [&]() {
    if(faces.empty())
      return;

    // merge meshes by material
    const int material = materialNameToId[currMaterial];

    if(material >= (int)result.plainMeshes.size())
      result.plainMeshes.resize(material + 1);

    auto& plainMesh = result.plainMeshes[material];
    for(auto v : faces)
      plainMesh.vertices.push_back(v);
    plainMesh.material = material;

    faces.clear();
  };

  Stream s{contents.data(), (int)contents.size()};

  while(s.len > 0)
  {
    auto line = parseLine(s);

    if(line.len == 0 || line.ptr[0] == '#')
      continue;

    std::string cmd = parseWord(line);

    if(cmd == "v")
    {
      flushFaces();
      float3 v{};
      sscanf(line.ptr, "%f %f %f", &v.x, &v.y, &v.z);
      coords.push_back(v);
    }
    else if(cmd == "vn")
    {
      float3 n{};
      sscanf(line.ptr, "%f %f %f", &n.x, &n.y, &n.z);
      normals.push_back(n);
    }
    else if(cmd == "f")
    {
      int c0, c1, c2;
      int n0, n1, n2;

      int r = sscanf(line.ptr, "%d//%d %d//%d %d//%d", &c0, &n0, &c1, &n1, &c2, &n2);
      assert(r == 6);

      c0--;
      c1--;
      c2--;

      n0--;
      n1--;
      n2--;

      faces.push_back({coords[c0].x, coords[c0].y, coords[c0].z, normals[n0].x, normals[n0].y, normals[n0].z});
      faces.push_back({coords[c1].x, coords[c1].y, coords[c1].z, normals[n1].x, normals[n1].y, normals[n1].z});
      faces.push_back({coords[c2].x, coords[c2].y, coords[c2].z, normals[n2].x, normals[n2].y, normals[n2].z});
    }
    else if(cmd == "usemtl")
    {
      auto word = parseWord(line);
      std::string material(word.ptr, word.len);

      if(currMaterial != material)
        flushFaces();

      currMaterial = material;
    }
    else if(cmd == "mtllib")
    {
      std::string libPath;
      std::istringstream iss(std::string(line.ptr, line.len));
      iss >> libPath;

      std::string dir = path;
      auto i = dir.rfind("/");
      dir = dir.substr(0, i);
      libPath = dir + "/" + libPath;

      for(auto& i : loadMaterialLib(libPath.c_str()))
      {
        int id = (int)result.materials.size();
        materialNameToId[i.first] = id;
        result.materials.push_back(i.second);
      }
    }
  }

  return result;
}
