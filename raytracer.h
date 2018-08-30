#ifndef RAYTRACER_H
#define RAYTRACER_H

#include "constants.h"

#include <vector>

#include "vec3.h"
#include "ray.h"
#include "shapes.h"
#include "light.h"
#include "kdtree.h"
#include "photonmap.h"
#include "randomc/randomc.h"

const int K_NEAREST_AMT = 100;

class Raytracer{
 public:
    // Required by the serialization library and input processing.
    Raytracer() {}

    // Instantiate a raytracer with all the necessary information.
    Raytracer(const Vec3&, const Vec3&, float, int, int, float, int, int, const Color&, const Color&, const vector<Light>&, const vector<Shape*>&);
    
    // Compute the color at the given pixel location (bottom-left origin). This is done
    // with one or more calls to colorTrace(Ray, int), depending on how many samples
    // are being use for anti-aliasing (if any).
    Color colorTrace(int, int, CRandomMersenne&) const;

    // Get the X or Y resolution or the antialias samples of the image.
    int getX() const;
    int getY() const;
    int getAASamples() const;

 private:
    // Compute the color for the given ray, returning a default color if the depth
    // has gone too far.
    Color colorTrace(const Ray&, CRandomMersenne&, int depth = 0) const;

    // Return true if there are any objects between the two given points, false otherwise.
    bool booleanTrace(const Vec3&, const Vec3&) const;

    // Get the radiance of the nearest object from the photon map.
    Color radianceTrace(const Ray&) const;

    // Trace the photon given by the Ray and the Color, putting entries into the map as appropriate.
    void photonTrace(const Color&, const Ray&, CRandomMersenne&, int, bool, vector<Photon>&, vector<Photon>&) const;

    // Initialize the photon map.
    void createPhotonMap(int, const vector<Light>&);

    // Divide the pixel into a square grid with aa_samples spaces on a side. The number of
    // rays used grows quadratically with this value.
    int aa_samples;

    // The resolution of the output image.
    int resx, resy;

    // The location of the camera eye.
    Vec3 eye;

    // The bottom-left of the pixel grid, in world coordinates.
    Vec3 origin;

    // dx and dy for the pixel grid. Allows mapping of pixel locations onto world
    // coordinates. Not necessarily multiples of <1, 0, 0> and <0, 1, 0>.
    Vec3 cam_x_vec, cam_y_vec;

    // The background color used when no shapes are hit or maximum recursion depth is
    // reached.
    Color bkrd;

    // The ambient light existing in the scene.
    Color ambient;

    // The kd-tree that keeps track of all objects that exist in the scene and allows very
    // fast access to them.
    KDNode kdtree;

    // Whether or not we are using the photon map for this ray trace.
    bool using_photons;
    
    // The global illumination, non-caustics photon map.
    PhotonMap global_map;

    // The caustics photon map.
    PhotonMap caustics_map;

    // All the lights that have been set for the scene.
    vector<Light> lights;

    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & aa_samples;
        ar & resx;
        ar & resy;
        ar & eye;
        ar & origin;
        ar & cam_x_vec;
        ar & cam_y_vec;
        ar & bkrd;
        ar & ambient;
        ar & kdtree;
        ar & using_photons;
        ar & global_map;
        ar & caustics_map;
        ar & lights;
    }
};

#endif
