#ifndef MATERIAL_H
#define MATERIAL_H

#include "constants.h"

#include "vec3.h"

// A class representing the materials objects are made of. Later versions will
// include support for texture mapping.
class Material{
 public:
    // The color of this material.
    Color color; 

    // How strongly this source reflects diffusely or specularly, on [0, 1].
    float k_diffuse, k_specular;

    // Percentage of the light hitting this material that is reflected or refracted. 
    // pct_refl + pct_refr must be <= 1.
    float pct_refl, pct_refr; 

    // The shininess exponent (Phong lightning model).
    float shininess; 

    // The refactive index.
    float refr_index;

 private:
    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & color;
        ar & k_diffuse;
        ar & k_specular;
        ar & pct_refl;
        ar & pct_refr;
        ar & shininess;
        ar & refr_index;
    }
};

#endif
