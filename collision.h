#ifndef COLLISION_H
#define COLLISION_H

#include "vec3.h"

class Shape;

// Collision structs represent all important information regarding a ray-object
// collision in an object-independent way. A pointer is still maintained to the
// collided shape -- it doesn't matter what shape the shape has, but we do want
// to be able to access its material and other properties common to all shapes.
struct Collision{
    Collision() : collided(false) {}
    Collision(Shape const *s) : collided(false), shape(s) {}

    // If this is false, the contents of the other variables are undefined.
    bool collided;

    // Variable pointer to the const Shape that this is a collision with.
    Shape const *shape;

    // t-value along the ray that generated this Collision.
    float distance; 

    // Normal to the surface of the shape at the collision point.
    Vec3 normal;
};

#endif
