#ifndef PROCESSINPUT_H
#define PROCESSINPUT_H

#include "constants.h"

#include <fstream>
#include <map>

#include "vec3.h"
#include "material.h"
#include "raytracer.h"

// Return types for processArguments, dictating what type of program this has
// been instatiated as.
enum {LOCAL, SERVER, CLIENT};

// Ensure the material with the given name exists.
void checkMaterialName(map<string, Material>&, char*);

// Ensure the given float is between 0 and 1. If not, use the supplied string
// to print a helpful error message.
void checkFloat0_1(float, string);

// Ensure the each of the vector's elements are between 0 and 1. If not, use the
// supplied string to print a helpful error message.
void checkVector0_1(Vec3&, string);

// Ensure the given float is positive. If not, use the supplied string to print
// a helpful error message.
void checkFloatIsPositive(float, string);

// Skip along the stream until the given tag is found. Allows easy skipping of
// comments and other lines without useful information.
void goToTag(istream&, string);

// Process the test stream into a ready-to-go Raytracer object.
Raytracer processInput(istream&);

// Process the command line arguments, returning the type of program this is
// instantiated as and filling the supplied arguments with the user-input
// values, if they exist (and defaults otherwise).
int processArguments(int, char**, string&, string&, string&, uint8_t&);

#endif
