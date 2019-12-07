/**
 * Copyright (c) 2012-2019 the Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

// Simple 2D vector library
#ifndef VEC_H_
#define VEC_H_

#include <stdbool.h>
#include <math.h>

typedef double vec_dimension;

// Forward definition of Line to avoid needing to circularly include Line.h
//struct Line;

// A two-dimensional vector.
struct Vec {
  vec_dimension x;  // The x-coordinate of the vector.
  vec_dimension y;  // The y-coordinate of the vector.
};
typedef struct Vec Vec;

// An axis-aligned rectangle.
struct Rect {
  vec_dimension xmin;  // The minimum x-coordinate of the rectangle.
  vec_dimension ymin;  // The minimum y-coordinate of the rectangle.
  vec_dimension xmax;  // The maximum x-coordinate of the rectangle.
  vec_dimension ymax;  // The maximum y-coordinate of the rectangle.
};
typedef struct Rect Rect;

// The allowable colors for a line.
typedef enum {
  RED = 0,
  GRAY = 1
} Color;

// A two-dimensional line.
struct Line {
  Vec p1;  // One endpoint of the line.
  Vec p2;  // The other endpoint of the line.

  // The line's current velocity, in units of pixels per time step.
  Vec velocity;
  Rect rectangle;

  Color color;  // The line's color.

  unsigned int id;  // Unique line ID.
};
typedef struct Line Line;



// Returns a vector with the specified x and y coordinates.
static inline Vec Vec_make(const vec_dimension x, const vec_dimension y) {
  Vec vector;
  vector.x = x;
  vector.y = y;
  return vector;
}

// ************************* Fundamental attributes **************************

// Returns the magnitude of the vector.
static inline vec_dimension Vec_length(Vec vector) {
  return hypot(vector.x, vector.y);
}

// Returns the argument of the vector - that is, the angle it makes with the
// positive x axis.  Units are radians.
static inline double Vec_argument(Vec vector) {
  return atan2(vector.y, vector.x);
}

// ******************************* Arithmetic ********************************

static inline bool Vec_equals(Vec lhs, Vec rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

static inline Vec Vec_add(Vec lhs, Vec rhs) {
  return Vec_make(lhs.x + rhs.x, lhs.y + rhs.y);
}

static inline Vec Vec_subtract(Vec lhs, Vec rhs) {
  return Vec_make(lhs.x - rhs.x, lhs.y - rhs.y);
}

static inline Vec Vec_multiply(Vec vector, const double scalar) {
  return Vec_make(vector.x * scalar, vector.y * scalar);
}

static inline Vec Vec_divide(Vec vector, const double scalar) {
  return Vec_make(vector.x / scalar, vector.y / scalar);
}

// Computes the dot product of two vectors.
static inline vec_dimension Vec_dotProduct(Vec lhs, Vec rhs) {
  return lhs.x * rhs.x + lhs.y * rhs.y;
}

// Computes the magnitude of the cross product of two vectors.
static inline vec_dimension Vec_crossProduct(Vec lhs, Vec rhs) {
  return lhs.x * rhs.y - lhs.y * rhs.x;
}

// Returns a vector parallel to the provided Line.  The direction of the
// vector is unspecified.
static inline Vec Vec_makeFromLine(struct Line line) {
  return Vec_subtract(line.p1, line.p2);
}

// **************************** Related vectors ******************************

// Returns a unit vector parallel to the vector.
static inline Vec Vec_normalize(Vec vector) {
  return Vec_divide(vector, Vec_length(vector));
}

// Returns a vector identical in magnitude and perpendicular to the vector.
static inline Vec Vec_orthogonal(Vec vector) {
  return Vec_make(-vector.y, vector.x);
}

// ******************** Relationships with other vectors *********************

// Computes the angle between vector1 and vector2.
static inline double Vec_angle(Vec vector1, Vec vector2) {
  return Vec_argument(vector1) - Vec_argument(vector2);
}

// Computes the scalar component of vector1 onto vector2.
static inline vec_dimension Vec_component(Vec vector1, Vec vector2) {
  return Vec_length(vector1) * cos(Vec_angle(vector1, vector2));
}

// Returns the vector projection of vector1 onto vector2.
static inline Vec Vec_projectOnto(Vec vector1, Vec vector2) {
  return Vec_multiply(Vec_normalize(vector2), Vec_component(vector1, vector2));
}

#endif  // VEC_H_
