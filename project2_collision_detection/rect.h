#ifndef RECT_H_
#define RECT_H_

#include <math.h>
#include <stdbool.h>
#include "./vec.h"
#include "./line.h"

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

// Axis-aligned rectangle for bounding the line over a timestep

// Returns a rectangle with the specified x and y coordinates.
static inline Rect Rect_make(const vec_dimension xmin, const vec_dimension xmax, const vec_dimension ymin, const vec_dimension ymax) {
  Rect rectangle;
  rectangle.xmin = xmin;
  rectangle.xmax = xmax;
  rectangle.ymin = ymin;
  rectangle.ymax = ymax;
  return rectangle;
}

// Returns a rectangle to bound the provided Line over the timestep.
static inline Rect Rect_makeFromLine(struct Line* line, double timestep) {
  Vec p1 = line->p1;
  Vec p2 = line->p2;
  Vec shift = Vec_multiply(line->velocity, timestep);
  Vec p3 = Vec_add(p1, shift);
  Vec p4 = Vec_add(p2, shift);

  // find the bounds of the line over the duration of the timestep
  double max_x = max(max(p1.x, p2.x), max(p3.x, p4.x));
  double min_x = min(min(p1.x, p2.x), min(p3.x, p4.x));
  double max_y = max(max(p1.y, p2.y), max(p3.y, p4.y));
  double min_y = min(min(p1.y, p2.y), min(p3.y, p4.y));

  // make a rectangle with sides at the specified coordinates
  return Rect_make(min_x, max_x, min_y, max_y);
}

#endif  // RECT_H_
