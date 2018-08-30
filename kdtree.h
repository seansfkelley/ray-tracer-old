#ifndef KDTREE_H
#define KDTREE_H

#include "constants.h"
#include "shapes.h"

// Needs a destructor.

// The smallest number of shapes a node can hold before it's a candidate for splitting.
const unsigned int KD_SPLIT_THRESHOLD = 4;

class KDNode{
 public:
    // Required by the serialization library indirectly through Raytracer.
    KDNode() {}

    // Create a new kd-tree with the given list of Shapes, that assigns itself
    // a volume large enough to surround the given shapes. It automatically
    // begins the splitting process.
    KDNode(const vector<Shape*>&);

    // Create a child node with the given low and high corners that
    // automatically begins the splitting process.
    KDNode(const vector<Shape*>&, const Vec3&, const Vec3&);

    // Collide the ray with the KDNode. If this is a leaf node, collide the ray with the
    // objects and update the given Collision if appropriate. Otherwise, delegate the
    // colliding to the children nodes.
    void collide(const Ray&, Collision&) const;

    // Return true if the ray hits an object within the given distance for the given ray
    // using the same algorithm as the normal collide function, but terminating as early
    // as possible.
    bool collideBoolean(const Ray&, const float) const;

    // Print out this KDNode and all its children.
    void print(const int) const;

 private:
    // Called by collide() when it knows the ray in question only hits one of the children.
    void collideOneChild(const Ray&, Collision&) const;

    // Corresponding collideOneChild, but for collideBoolean.
    bool collideOneChildBoolean(const Ray&, const float) const;

    // Split this KDNode into 2 new children, populate them, then ask them to
    // split if appropriate.
    void split();

    // Set the bounds of this node to surround all the contained shapes.
    void calculateAggregateBounds();

    // The bounds of this node. low_corner < high_corner for all elements.
    Vec3 low_corner, high_corner;

    // The location of the splitting plane in world coordinates (along axis 'axis').
    float partition_distance;

    // Which axis this node is split over, or LEAF if it isn't.
    int axis;

    // The children of this node, if they exist.
    KDNode *children[2];

    // What shapes this leaf node contains.
    vector<Shape*> shapes;

    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & low_corner;
        ar & high_corner;
        ar & partition_distance;
        ar & axis;
        ar & shapes;
        ar & children;
    } 
};

#endif
