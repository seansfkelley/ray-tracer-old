#include "shapes.h"

#include <cmath>

inline void clampVector(Vec3 &v, const Vec3 &min, const Vec3 &max){
    if (v.x < min.x) v.x = min.x;
    else if (v.x > max.x) v.x = max.x;
    if (v.y < min.y) v.y = min.y;
    else if (v.y > max.y) v.y = max.y;
    if (v.z < min.z) v.z = min.z;
    else if (v.z > max.z) v.z = max.z;
}

Sphere::Sphere(const Material &material, const Vec3 &c, float r) : Shape(material), center(c), rad(fabs(r)) {}

Collision Sphere::collide(const Ray &r) const{
    Collision c(this);

    Vec3 t = r.origin - center;

    // Shortcut quadratic formula.
    float B = -t.dot(r.direction);
    float D = B * B - t.magnitude2() + rad * rad;

    if (D > 0){
        D = sqrt(D);
        c.distance = B - D < 0 ? B + D : B - D;
        if (c.distance > 0){
            c.collided = true;
            c.normal = (r.pointAt(c.distance) - center).asNormal();
            // Flip the normal if the ray began inside the sphere.
            if ((r.origin - center).magnitude2() < rad * rad){
                c.normal = -c.normal;
            }
        }
    }
    
    return c;
}

bool Sphere::collidesWithBox(const Vec3 &box_low_corner, const Vec3 &box_high_corner) const{
    Vec3 clamped_center = center;
    clampVector(clamped_center, box_low_corner, box_high_corner);
    return (clamped_center - center).magnitude2() < (rad * rad);
}

float Sphere::extremeValue(uint8_t axis, bool largest) const{
    return largest ? center[axis] + rad : center[axis] - rad;
}

BOOST_CLASS_EXPORT_IMPLEMENT(Sphere)

RectPrism::RectPrism(const Material &material, const Vec3 &position, const Vec3 &dimensions) : Shape(material){
    low_corner = position;
    // By enforcing bounds_min <= bounds_max for each element, collision becomes significantly simpler.
    for (int i = 0; i < 3; ++i){
        if (dimensions[i] < 0){
            low_corner[i] += dimensions[i];
        }
        high_corner[i] = position[i] + fabs(dimensions[i]);
    }
}

Collision RectPrism::collide(const Ray &r) const{
    Collision c(this);
    // Do backface culling if we aren't currently inside an object.
    bool do_culling = r.inside_shape == NULL;

    for (int corner = 0; corner < 2; ++corner){
        for (int i = 0; i < 3; ++i){
            if (do_culling){
                Vec3 normal(0, 0, 0);
                normal[i] = 1 - 2 * corner;
                if (r.direction.dot(normal) > 0){
                    continue;
                }
            }
            // A lot of the math here is significantly simplified because we're using axis-aligned boxes. Since
            // the faces of these boxes have either <1,0,0>, <0,1,0> or <0,0,1> as their normal, instead of
            // dotting a vector with the normal we can simply pull out the appropriate coordinate (x, y and z
            // respectively). The (r.direction[i] != 0) check represents checking if the denominator of the
            // expression is zero, which indicates that the ray is parallel to the plane in question. The 't ='
            // statement does all the numerator/denominator calculation at once. opposite_corner > pos, as
            // enforced by the constructor, so we always know which case is being referred to by corner being
            // 0 or 1.
            if (r.direction[i] != 0){
                // One collision somewhere on the plane. Enforce boundaries of the face.
                float t = (corner == 0 ? high_corner[i] - r.origin[i] : low_corner[i] - r.origin[i]) / r.direction[i];
                if (t < 0){
                    // Behind the camera.
                    continue;
                }
                Vec3 point = r.pointAt(t);
                // Move into the prism EPSILON amount to fix rounding errors.
                point[i] -= (1 - 2 * corner) * EPSILON;
                // We know pos <= opposite_corner, so we only have to check in this direction.
                if ((c.collided == false || t < c.distance) && 
                    low_corner <= point && point <= high_corner){
                    c.collided = true;
                    c.distance = t;
                    c.normal = Vec3(0, 0, 0);
                    c.normal[i] = 1 - 2 * corner;
                }
            }
        }
    }
    
    // Flip the normal if the ray began inside the prism.
    if (c.collided && low_corner <= r.origin && r.origin <= high_corner){
        c.normal = -c.normal;
    }

    return c;
}

bool RectPrism::collidesWithBox(const Vec3 &box_low_corner, const Vec3 &box_high_corner) const{
    return !(low_corner.x > box_high_corner.x ||
             box_low_corner.x > high_corner.x ||
             low_corner.y > box_high_corner.y ||
             box_low_corner.y > high_corner.y ||
             low_corner.z > box_high_corner.z ||
             box_low_corner.z > high_corner.z);
}

float RectPrism::extremeValue(uint8_t axis, bool largest) const{
    return largest ? high_corner[axis] : low_corner[axis];
}

BOOST_CLASS_EXPORT_IMPLEMENT(RectPrism)
