#ifndef OCTREE_H
#define OCTREE_H

#include "constants.h"

#include <vector>
#include <set>

#include "shapes.h"
#include "vec3.h"
#include "ray.h"

// How many shapes a node can hold before it splits.
const unsigned int SPLIT_THRESHOLD = 16;

// This class is designed to recieve a list of all objects at once.
// It is not designed for modification after initialization.

// Note: don't return an object twice in one query. This might happen
// because its bounding box intersects multiple octrees. This check
// is not yet implemented.
class OctreeNode{
 public:
    // Required by the serialization library. Do not call this constructor.
    OctreeNode() {}

    // Create a new octree with the given list of Shapes.
    OctreeNode(const vector<Shape*>&);

    // Called when splitting an octree node. Do not call this constructor directly.
    OctreeNode(const Vec3&, const Vec3&, const vector<Shape*>&);

    // Collide the ray with the octree and return a list of possible Shape collisions.
    void collide(const Ray&, vector<Shape*>&) const;

    // Print this octree out.
    void print(int depth = 0) const;

 private:
    // Split this octree node into 8 new children, populate them, then ask them to
    // split if appropriate.
    void split();

    // Helper function when performing splits. Places the given shape into each of
    // the given vectors (one per child) where it belongs.
    void shapeIntersections(Shape*, vector<Shape*>*);

    // Figure out which octant the given point falls into.
    unsigned int octantOfPoint(const Vec3&) const;

    // Center this octree on the center point of all the contained shapes' bounds.
    // An easy way to get the octree to be "pretty balanced".
    void centerOnAggregateBounds();

    // The bounds of this node. low_corner < high_corner and 
    // center = (low_corner + high_corner) / 2.
    Vec3 low_corner, center, high_corner;

    // Is this node a leaf or not?
    bool is_split;

    // The children of this node, if they exist.
    OctreeNode *children[8];

    // What shapes this leaf node contains.
    vector<Shape*> shapes;

    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & low_corner;
        ar & center;
        ar & high_corner;
        ar & is_split;
        ar & children;
        ar & shapes;
    }
};

#endif
