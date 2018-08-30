#include "light.h"

void Light::setArea(const float side_length, const int samples){
    if (samples == 1 || side_length == 0){
        area_light = false;
    }
    else{
        this->samples = samples;
        
        x_vec = Vec3(side_length / samples, 0, 0);
        z_vec = Vec3(0, 0, side_length / samples);
        
        Vec3 offset_from_pos(side_length / 2, 0, side_length / 2);
        corner = pos - offset_from_pos;
        opposite_corner = pos + offset_from_pos;

        area_light = true;
    }
}

vector<Vec3> Light::samplePoints(CRandomMersenne& twister) const{
    vector<Vec3> points;
    if (!area_light){
        points.push_back(pos);
        return points;
    }
    
    for (int x = 0; x < samples; ++x){
        for (int z = 0; z < samples; ++z){
            points.push_back(corner + x_vec * (x + (float) twister.Random()) + z_vec * (z + (float) twister.Random()));
        }
    }

    return points;
}

float Light::collide(const Ray &r) const{
    if (!area_light){
        return -1;
    }

    if (r.direction.y != 0){
        float t = (pos.y - r.origin.y) / r.direction.y;
        if (t < 0){
            return -1;
        }
        Vec3 collision_point = r.pointAt(t);
        collision_point.y = corner.y; // Remove the possibility of rounding errors. The y term might be computed differently
                                      // by pointAt, but we know it must lie in the plane of the light.
        return corner <= collision_point && collision_point <= opposite_corner ? t : -1;
    }
    
    return -1;
}
