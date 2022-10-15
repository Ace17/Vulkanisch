#pragma once

#include <stddef.h>

static const double PI = 3.14159265358979323846;

template<typename T>
inline T lerp(const T& a, const T& b, float alpha)
{
  return a * (1.0f - alpha) + b * alpha;
}

template<typename T>
inline T clamp(const T& val, const T& min, const T& max)
{
  if(val < min)
    return min;

  if(val > max)
    return max;

  return val;
}

///////////////////////////////////////////////////////////////////////////////
// Vec2f
///////////////////////////////////////////////////////////////////////////////

struct Vec2f
{
  float x = 0;
  float y = 0;

  Vec2f() = default;
  Vec2f(float x_, float y_)
      : x(x_)
      , y(y_)
  {
  }

  Vec2f operator*(float f) const { return {x * f, y * f}; }
};

inline Vec2f operator+(Vec2f a, Vec2f b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2f operator-(Vec2f a, Vec2f b) { return {a.x - b.x, a.y - b.y}; }

///////////////////////////////////////////////////////////////////////////////
// Vec3f
///////////////////////////////////////////////////////////////////////////////

struct Vec3f
{
  float x = 0;
  float y = 0;
  float z = 0;

  Vec3f() = default;
  Vec3f(float x_, float y_, float z_)
      : x(x_)
      , y(y_)
      , z(z_)
  {
  }

  Vec3f operator+=(Vec3f const& other)
  {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }

  Vec3f operator-=(Vec3f const& other)
  {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }

  template<typename F>
  friend Vec3f operator*(Vec3f const& a, F val)
  {
    return Vec3f(a.x * val, a.y * val, a.z * val);
  }

  template<typename F>
  friend Vec3f operator*(F val, Vec3f const& a)
  {
    return Vec3f(a.x * val, a.y * val, a.z * val);
  }

  friend Vec3f operator+(Vec3f const& a, Vec3f const& b)
  {
    Vec3f r = a;
    r += b;
    return r;
  }

  friend Vec3f operator-(Vec3f const& a, Vec3f const& b)
  {
    Vec3f r = a;
    r -= b;
    return r;
  }

  bool operator==(Vec3f const& other) const { return x == other.x && y == other.y && z == other.z; }
};

inline auto dotProduct(Vec3f a, Vec3f b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

double magnitude(Vec3f v);

inline auto normalize(Vec3f v) { return v * (1.0f / magnitude(v)); }

inline auto crossProduct(Vec3f a, Vec3f b)
{
  Vec3f r;
  r.x = a.y * b.z - a.z * b.y;
  r.y = a.z * b.x - b.z * a.x;
  r.z = a.x * b.y - a.y * b.x;
  return r;
}

///////////////////////////////////////////////////////////////////////////////
// Vec4f
///////////////////////////////////////////////////////////////////////////////

struct Vec4f
{
  float x, y, z, w;
};
