#include <cassert>
#include <cmath>

#include "matrix4.h"

double magnitude(Vec3f v) { return std::sqrt(dotProduct(v, v)); }

Matrix4f::Matrix4f(float init)
{
  for(int row = 0; row < 4; ++row)
    for(int col = 0; col < 4; ++col)
      (*this)[col][row] = init;
}

Matrix4f operator*(Matrix4f const& A, Matrix4f const& B)
{
  Matrix4f r(0);

  for(int row = 0; row < 4; ++row)
    for(int col = 0; col < 4; ++col)
    {
      double sum = 0;

      for(int k = 0; k < 4; ++k)
        sum += A[row][k] * B[k][col];

      r[row][col] = sum;
    }

  return r;
}

Vec4f operator*(Matrix4f const& A, Vec4f v)
{
  Vec4f r;

  r.x = A[0][0] * v.x + A[0][1] * v.y + A[0][2] * v.z + A[0][3] * v.w;
  r.y = A[1][0] * v.x + A[1][1] * v.y + A[1][2] * v.z + A[1][3] * v.w;
  r.z = A[2][0] * v.x + A[2][1] * v.y + A[2][2] * v.z + A[2][3] * v.w;
  r.w = A[3][0] * v.x + A[3][1] * v.y + A[3][2] * v.z + A[3][3] * v.w;

  return r;
}

Matrix4f translate(Vec3f v)
{
  Matrix4f r(0);

  r[0][0] = 1;
  r[1][1] = 1;
  r[2][2] = 1;
  r[3][3] = 1;

  r[0][3] = v.x;
  r[1][3] = v.y;
  r[2][3] = v.z;

  return r;
}

Matrix4f scale(Vec3f v)
{
  Matrix4f r(0);
  r[0][0] = v.x;
  r[1][1] = v.y;
  r[2][2] = v.z;
  r[3][3] = 1;
  return r;
}

Matrix4f transpose(const Matrix4f& m)
{
  Matrix4f r(0);

  for(int row = 0; row < 4; ++row)
    for(int col = 0; col < 4; ++col)
      r[row][col] = m[col][row];

  return r;
}

Matrix4f rotateX(float angle)
{
  const auto c = cos(angle);
  const auto s = sin(angle);

  Matrix4f r(0);

  r[0][0] = 1;

  r[1][1] = c;
  r[1][2] = -s;

  r[2][1] = s;
  r[2][2] = c;

  r[3][3] = 1;

  return r;
}

Matrix4f rotateY(float angle)
{
  const auto c = cos(angle);
  const auto s = sin(angle);

  Matrix4f r(0);

  r[1][1] = 1;

  r[0][0] = c;
  r[0][2] = s;

  r[2][0] = -s;
  r[2][2] = c;

  r[3][3] = 1;

  return r;
}

Matrix4f rotateZ(float angle)
{
  const auto c = cos(angle);
  const auto s = sin(angle);

  Matrix4f r(0);

  r[2][2] = 1;

  r[0][0] = c;
  r[0][1] = -s;

  r[1][0] = s;
  r[1][1] = c;

  r[3][3] = 1;

  return r;
}

// Inverts a transform/rotate/scale matrix.
//
// [ux vx wx tx]      [ux uy uz -dot(u,t)]
// [uy vy wy ty] ---> [vx vy vz -dot(v,t)]
// [uz vz wz tz]      [wx wy wz -dot(w,t)]
// [ 0  0  0  1]      [ 0  0  0     1    ]
Matrix4f invertStandardMatrix(const Matrix4f& m)
{
  const Vec3f u(m[0][0], m[1][0], m[2][0]);
  const Vec3f v(m[0][1], m[1][1], m[2][1]);
  const Vec3f w(m[0][2], m[1][2], m[2][2]);
  const Vec3f t(m[0][3], m[1][3], m[2][3]);

  Matrix4f r(0);

  r[0][0] = u.x;
  r[0][1] = u.y;
  r[0][2] = u.z;

  r[1][0] = v.x;
  r[1][1] = v.y;
  r[1][2] = v.z;

  r[2][0] = w.x;
  r[2][1] = w.y;
  r[2][2] = w.z;

  r[0][3] = -dotProduct(u, t);
  r[1][3] = -dotProduct(v, t);
  r[2][3] = -dotProduct(w, t);

  r[3][3] = 1;

  return r;
}

Matrix4f lookAt(Vec3f eye, Vec3f center, Vec3f up)
{
  auto f = normalize(center - eye);
  auto s = normalize(crossProduct(f, up));
  auto u = crossProduct(s, f);

  Matrix4f r(0);
  r[0][0] = s.x;
  r[0][1] = s.y;
  r[0][2] = s.z;
  r[1][0] = u.x;
  r[1][1] = u.y;
  r[1][2] = u.z;
  r[2][0] = -f.x;
  r[2][1] = -f.y;
  r[2][2] = -f.z;
  r[0][3] = -dotProduct(s, eye);
  r[1][3] = -dotProduct(u, eye);
  r[2][3] = dotProduct(f, eye);
  r[3][3] = 1;
  return r;
}

Matrix4f perspective(float fovy, float aspect, float zNear, float zFar)
{
  assert(aspect != 0.0);
  assert(zFar != zNear);

  auto const rad = fovy;
  auto const tanHalfFovy = tan(rad / 2.0);

  Matrix4f r(0);
  r[0][0] = 1.0 / (aspect * tanHalfFovy);
  r[1][1] = 1.0 / (tanHalfFovy);
  r[2][2] = -(zFar + zNear) / (zFar - zNear);
  r[3][2] = -1.0;
  r[2][3] = -(2.0 * zFar * zNear) / (zFar - zNear);
  return r;
}
