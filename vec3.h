#ifndef VEC3_H
#define VEC3_H

#include "constants.h"

#define CLAMP0_1(x) ((x) < 0 ? 0 : ((x) > 1 ? 1 : (x)))

// The entire class is automatically inlined!
class Vec3{
 public:
    union{
        struct { float x, y, z; };
        struct { float r, g, b; };
        float element[3];
    };

    Vec3() : x(0), y(0), z(0) {}
    Vec3(const Vec3 &v) : x(v.x), y(v.y), z(v.z) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    float& operator[](int index) { return element[index]; }
    const float operator[](int index) const { return element[index]; }

    Vec3 operator-() const { return Vec3(-x, -y, -z); }

    Vec3 operator*(float s) const { return Vec3(s * x, s * y, s * z); }
    Vec3 operator/(float s) const { s = 1 / s; return Vec3(s * x, s * y, s * z); }
    Vec3 operator+(const Vec3 &v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3 &v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    // Component-wise multiplication, not to be confused with the dot product.
    Vec3 operator*(const Vec3 &v) const { return Vec3(x * v.x, y * v.y, z * v.z); }

    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    Vec3& operator/=(float s) { s = 1 / s; x *= s; y *= s; z *= s; return *this; }
    Vec3& operator+=(const Vec3 &v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vec3& operator-=(const Vec3 &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    // Component-wise multiplication, not to be confused with the dot product.
    Vec3& operator*=(const Vec3 &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }

    bool operator< (const Vec3 &v) const { return x <  v.x && y <  v.y && z <  v.z; }
    bool operator<=(const Vec3 &v) const { return x <= v.x && y <= v.y && z <= v.z; }
    bool operator> (const Vec3 &v) const { return x >  v.x && y >  v.y && z >  v.z; }
    bool operator>=(const Vec3 &v) const { return x >= v.x && y >= v.y && z >= v.z; }
    bool operator==(const Vec3 &v) const { return x == v.x && y == v.y && z == v.z; }
    bool operator!=(const Vec3 &v) const { return x != v.x || y != v.y || z != v.z; }

    float dot(const Vec3 &v) const { return x*v.x + y*v.y + z*v.z; }
    Vec3 cross(const Vec3 &v) const { return Vec3(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x); }

    float magnitude() const { return sqrt(x*x + y*y + z*z); }
    float magnitude2() const { return x*x + y*y + z*z; }

    void normalize() { float m = 1 / magnitude(); x *= m; y *= m; z *= m; }

    Vec3 asNormal() const { float m = 1 / magnitude(); return Vec3(x * m, y * m, z * m); }

    // Return a new vector that is the same as this vector, with each element truncated to [0, 1].
    Vec3 asClamped0_1() const { return Vec3(CLAMP0_1(x), CLAMP0_1(y), CLAMP0_1(z)); }

    void print(bool newline = true) const { cout << '<' << x << ',' << y << ',' << z << '>'; if (newline) cout << endl; }

 private:
    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & x;
        ar & y;
        ar & z;
    }
};

typedef Vec3 Color;

#endif
