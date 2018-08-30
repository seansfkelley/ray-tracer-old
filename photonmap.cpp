#include "photonmap.h"

#include <limits>
#include <algorithm>
#include <stack>
#include <queue>
#include <utility>

#define LEFT 0
#define RIGHT 1

inline void clampVector(Vec3 &v, const Vec3 &min, const Vec3 &max){
    if (v.x < min.x) v.x = min.x;
    else if (v.x > max.x) v.x = max.x;
    if (v.y < min.y) v.y = min.y;
    else if (v.y > max.y) v.y = max.y;
    if (v.z < min.z) v.z = min.z;
    else if (v.z > max.z) v.z = max.z;
}

class PhotonComparator{
public:
    PhotonComparator(uint8_t axis) : axis(axis) {}

    bool operator()(const Photon &one, const Photon &two) { return one.point[axis] < two.point[axis]; }

    uint8_t axis;
};

PhotonMapNode::PhotonMapNode(Photon *photons, unsigned int length){
    if (length > 0){
        low_corner = photons[0].point, high_corner = photons[0].point;
        for (unsigned int i = 0; i < length; ++i){
            for (int j = 0; j < 3; ++j){
                low_corner[j] = min<float>(low_corner[j], photons[i].point[j]);
                high_corner[j] = max<float>(high_corner[j], photons[i].point[j]);
            }
        }
    }
    else{
        low_corner = high_corner = Vec3();
    }
    split(photons, 0, length - 1, X_AXIS);
}

PhotonMapNode::PhotonMapNode(Photon *photons, unsigned int left, unsigned int right, 
                             uint8_t axis, const Vec3 &low, const Vec3 &high) : low_corner(low), high_corner(high){
    split(photons, left, right, axis);
}

void PhotonMapNode::split(Photon *photons, unsigned int left, unsigned int right, uint8_t ax){
    if (right - left + 1 > MAX_PHOTONS_PER_NODE){
        axis = ax;
        sort(photons + left, photons + right + 1, PhotonComparator(axis));
        
        int median = (left + right) / 2;
        partition_distance = (photons[median].point[axis] + photons[median + 1].point[axis]) / 2;
        Vec3 mid_low_corner = low_corner, mid_high_corner = high_corner;
        mid_low_corner[axis] = mid_high_corner[axis] = partition_distance;
        children[LEFT] =  new PhotonMapNode(photons, left, median,      NEXT_AXIS(ax), low_corner, mid_high_corner);
        children[RIGHT] = new PhotonMapNode(photons, median + 1, right, NEXT_AXIS(ax), mid_low_corner, high_corner);
    }
    else{
        children[LEFT] = children[RIGHT] = NULL;
        partition_distance = 0;
        axis = LEAF;

        for (unsigned int i = 0; i < right - left + 1; ++i){
            p.push_back(photons + i + left);
        }
    }
}

PhotonMap::PhotonMap(Photon *photons, unsigned int length){
    root = new PhotonMapNode(photons, length);
}

void PhotonMap::kNearestNeighbors(const Vec3 &point, unsigned int k, vector<Photon*> &k_nearest) const{
    priority_queue<pair<float, Photon*> > nearest_photons;
    nearest_photons.push(pair<float, Photon*>(numeric_limits<float>::infinity(), NULL));
    stack<PhotonMapNode*> node_stack;
    node_stack.push(root);

    while (!node_stack.empty()){
        PhotonMapNode node = *node_stack.top();
        node_stack.pop();

        if (node.axis == LEAF){
            for (unsigned int i = 0; i < node.p.size(); ++i){
                nearest_photons.push(pair<float, Photon*>((node.p[i]->point - point).magnitude2(), node.p[i]));
            }
            while (nearest_photons.size() > k){
                nearest_photons.pop();
            }
        }
        else{
            Vec3 clamped_point = point;
            clampVector(clamped_point, node.low_corner, node.high_corner);
            if ((clamped_point - point).magnitude2() > nearest_photons.top().first){
                // Optimization: Because the largest distance may shrink between when this node was
                // pushed on the stack and when it was popped off and checked (now), the node might
                // not even intersect the largest sphere and neither of its children should be added.
                continue;
            }
            clamped_point[node.axis] = node.partition_distance;
            if ((clamped_point - point).magnitude2() < nearest_photons.top().first){
                node_stack.push(node.children[LEFT]);
                node_stack.push(node.children[RIGHT]);
            }
            else if (point[node.axis] < node.partition_distance){
                node_stack.push(node.children[LEFT]);
            }
            else{
                node_stack.push(node.children[RIGHT]);
            }
        }
    }

    if (isinf(nearest_photons.top().first)){
        // Didn't have as many photons as requested.
        nearest_photons.pop();
    }

    k_nearest.clear();
    k_nearest.reserve(min<int>(k, nearest_photons.size()));
    while (!nearest_photons.empty()){
        k_nearest.push_back(nearest_photons.top().second);
        nearest_photons.pop();
    }
}
