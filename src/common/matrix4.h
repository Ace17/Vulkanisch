#pragma once

#include "vec3.h"

struct Matrix4f
{
  Matrix4f(float init = 0);

  struct row
  {
    float const& operator[](int i) const { return elements[i]; }
    float& operator[](int i) { return elements[i]; }

    float elements[4];
  };

  const row& operator[](int i) const { return data[i]; }
  row& operator[](int i) { return data[i]; }

  row data[4];
};

Matrix4f operator*(Matrix4f const& A, Matrix4f const& B);
Vec4f operator*(Matrix4f const& A, Vec4f v);

Matrix4f translate(Vec3f v);
Matrix4f scale(Vec3f v);
Matrix4f transpose(const Matrix4f& m);

Matrix4f rotateX(float angle);
Matrix4f rotateY(float angle);
Matrix4f rotateZ(float angle);

// Inverts a transform/rotate/scale matrix.
//
// [ux vx wx tx]      [ux uy uz -dot(u,t)]
// [uy vy wy ty] ---> [vx vy vz -dot(v,t)]
// [uz vz wz tz]      [wx wy wz -dot(w,t)]
// [ 0  0  0  1]      [ 0  0  0     1    ]
Matrix4f invertStandardMatrix(const Matrix4f& m);
Matrix4f lookAt(Vec3f eye, Vec3f center, Vec3f up);
Matrix4f perspective(float fovy, float aspect, float zNear, float zFar);
