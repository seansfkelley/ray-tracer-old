#ifndef CONSTANTS_H
#define CONSTANTS_H

// Comment this line out to compile the client-only version that is not dependent on pngwriter.
#define HAVE_PNGWRITER

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#include "randomc/randomc.h"

using namespace std;

const float PI = 3.14159265;

// Debugging flag -- render only the photon maps instead of the real scene.
const bool RENDER_PHOTON_MAP_ONLY = false;

// Smaller numbers increase the power of lights in the photon mapping model.
const float GLOBAL_POWER_SCALING = 1.0 / 25000;

const float CAUSTICS_POWER_SCALING = 1.0 / 20000;

// Default number of work threads to use, both for networked and local.
const uint8_t DEFAULT_THREADS = 2; 

// Maximum depth to follow reflected rays.
const int MAX_REFLECTIONS = 12;

// Light falloff (attenuation). Higher = slower falloff.
const int FALLOFF = 160000;

// "Machine epsilon" to correct rounding errors in collision detection.
const float EPSILON = 1.0 / 100;

// Below this threshold, reflection calculated with Fresnel's equations is ignored.
const float FRESNEL_REFLECTIVE_MIN = 0.025;

// How many tiles pixels along one side of a tile. TILE_SIDE_LENGTH ^ 2 is the size of a tile.
// const int TILE_SIDE_LENGTH = 32;

// degrees * DEG_TO_RAD = radians
const float DEG_TO_RAD = 0.0174532925;

// Axes used for spatial indexing structures.
enum {X_AXIS = 0, Y_AXIS = 1, Z_AXIS = 2, LEAF = 3};

#define NEXT_AXIS(a) (((a) + 1) % 3)

#endif
