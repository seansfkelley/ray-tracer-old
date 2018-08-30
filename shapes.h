#ifndef SHAPE_H
#define SHAPE_H

#include <boost/serialization/export.hpp>

#include "constants.h"
#include "vec3.h"
#include "collision.h"
#include "ray.h"
#include "material.h"

#define EXTREME_VALUE_LARGEST true
#define EXTREME_VALUE_SMALLEST false

class Shape{
 public:
    Shape(const Material &material) : mat(material) {}
    
    // Collide this shape with the given ray and return the result.
    virtual Collision collide(const Ray&) const = 0;

    // Whether this shapes collides with the given rectangular prism. Used for
    // preprocessing the shapes into spatial indexing prisms as tightly as possible.
    virtual bool collidesWithBox(const Vec3&, const Vec3&) const = 0;

    // Get the largest/smallest value this shape achieves along the specified axis.
    virtual float extremeValue(uint8_t, bool) const = 0;

    // The material this shape is made of.
    Material mat;

 private:
    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & mat;
    }
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Shape)

class Sphere : public Shape{
 public:
    Sphere(const Material&, const Vec3&, float);

    Collision collide(const Ray&) const;
    bool collidesWithBox(const Vec3&, const Vec3&) const;
    float extremeValue(uint8_t, bool) const;

 private:
    // For serialization.
    Sphere() : Shape(Material()) {}

    // The center of this sphere.
    Vec3 center;

    // The radius of this sphere.
    float rad;

    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & boost::serialization::base_object<Shape>(*this);
        ar & center;
        ar & rad;
    }
};

BOOST_CLASS_EXPORT_KEY(Sphere)

class RectPrism : public Shape{
 public:
    RectPrism(const Material&, const Vec3&, const Vec3&);

    Collision collide(const Ray&) const;
    bool collidesWithBox(const Vec3&, const Vec3&) const;
    float extremeValue(uint8_t, bool) const;

 private:
    // For serialization.
    RectPrism() : Shape(Material()) {}

    // The two corners defining this prism. low_corner <= high_corner.
    Vec3 low_corner, high_corner;
    
    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & boost::serialization::base_object<Shape>(*this);
        ar & low_corner;
        ar & high_corner;
    }
};

BOOST_CLASS_EXPORT_KEY(RectPrism)

#endif
