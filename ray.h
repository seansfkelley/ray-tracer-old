#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class Shape;

class Ray{
 public:
    Ray() : inside_shape(NULL) {}
    Ray(const Vec3 &o, const Vec3 &d) : origin(o), direction(d), inside_shape(NULL) {}

    // Get the point along this ray at t.
    inline Vec3 pointAt(const float t) const {  return origin + (direction * t); }

    Vec3 origin, direction;
    
    // Which shape this ray is currently inside (or NULL if it's open air). Used for refraction.
    // This implementation assumes that two shapes cannot occupy the same physical space.
    Shape const *inside_shape;
};

#endif
