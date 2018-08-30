#ifndef PHOTONMAP_H
#define PHOTONMAP_H

#include "constants.h"

#include "vec3.h"
#include "ray.h"

const unsigned int MAX_PHOTONS_PER_NODE = 4; // Cannot be larger than 255!

struct Photon{
    Vec3 point, incident_direction, normal;
    Color color;

 private:
    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & point;
        ar & incident_direction;
        ar & normal;
        ar & color;
    } 
};

class PhotonMapNode{
 private:
    // Required by the serialization library.
    PhotonMapNode() {}

    // Create a new kd-tree rooted at this node.
    PhotonMapNode(Photon*, unsigned int);

    // Create a new kd-tree with the subarray of the given array defined by
    // the two given indices (INCLUSIVE) and the given axis. Used when 
    // constructing the tree recursively.
    PhotonMapNode(Photon*, unsigned int, unsigned int, uint8_t axis, const Vec3&, const Vec3&);

    // Split this PhotonMapNode into 2 new children over this node's axis.
    void split(Photon*, unsigned int, unsigned int, uint8_t);

    // This node's photons.
    vector<Photon*> p;

    // The children of this node, if they exist.
    PhotonMapNode *children[2];

    // The bounds of this node in world coodinates.
    Vec3 low_corner, high_corner;

    // The location of the splitting plane in world coordinates (along axis 'axis').
    float partition_distance;

    // Which axis this node is split over, or LEAF if it isn't.
    uint8_t axis;

    friend class PhotonMap;

    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & p;
        ar & low_corner;
        ar & high_corner;
        ar & partition_distance;
        ar & axis;
        ar & children;
    } 
};

class PhotonMap{
 public:
    // Instantiate an empty photon map.
    PhotonMap() {}

    PhotonMap(Photon*, unsigned int);

    // Find the k nearest neighbors to the given point and replace the contents
    // of the supplied vector with them, with the farthest photon first.
    void kNearestNeighbors(const Vec3&, unsigned int, vector<Photon*>&) const;

 private:
    // The root of this kd-tree photon map.
    PhotonMapNode *root;

    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & root;
    } 
};

#endif
