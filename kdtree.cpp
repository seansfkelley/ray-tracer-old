#include "kdtree.h"

#include <algorithm>
#include <set>
#include <cassert>
#include <limits>

#define LEFT 0
#define RIGHT 1

// long kd_recurses = 0, objects_checked = 0;

// Stores all the information for a potential partition in the kd-tree.
struct PartitionInfo{
    PartitionInfo(const Vec3& low_corner, const Vec3& high_corner, const int axis, const float distance) : count_left(0), 
                                                                                                           count_right(0), 
                                                                                                           partition_distance(distance){
        Vec3 mid_low_corner = low_corner,
             mid_high_corner = high_corner;
        mid_low_corner[axis] = mid_high_corner[axis] = distance;
        Vec3 left_dims = mid_high_corner - low_corner,
             right_dims = high_corner - mid_low_corner;
        surface_area_left = 2 * (left_dims.x * left_dims.y + left_dims.y * left_dims.z + left_dims.z * left_dims.x);
        surface_area_right = 2 * (right_dims.x * right_dims.y + right_dims.y * right_dims.z + right_dims.z * right_dims.x);
    }

    bool operator<(const PartitionInfo& other) const{
        return partition_distance < other.partition_distance;
    }

    int count_left, count_right;
    float partition_distance, surface_area_left, surface_area_right;
};

// Return the index of the partition with the given distance. Due to the nature of the creation of the vector, 
// it's guaranteed to be unique if it exists. If it doesn't, then it can only ever be smaller than the first
// element or larger than the last, so we return -1 or partitions.size() (before the first and after the last
// indices, respectively) in the appropriate case. Additionally, the values we are comparing will always be
// exactly equal if they're the one we want (even though they are floats).
int binarySearchPartitions(const vector<PartitionInfo>& partitions, const float distance){
    if (distance < partitions.front().partition_distance){
        return -1;
    }
    else if (distance > partitions.back().partition_distance){
        return partitions.size();
    }
    unsigned left = 0, right = partitions.size() - 1, index;
    while (true){
        // Quick division by two using bit shift!
        index = (left + right) >> 1;
        if (partitions[index].partition_distance == distance){
            return index;
        }
        else if (partitions[index].partition_distance > distance){
            right = index - 1;
        }
        else{
            left = index + 1;
        }
    }
}

KDNode::KDNode(const vector<Shape*>& s) : shapes(s){
    if (s.size() > 0){
        calculateAggregateBounds();
    }
    else{
        low_corner = high_corner = Vec3();
    }
    split();
}

KDNode::KDNode(const vector<Shape*>& s, const Vec3& low, const Vec3& high) : low_corner(low), high_corner(high), shapes(s){
    split();
}

void KDNode::print(const int depth) const{
    if (axis == LEAF){
        cout << string(depth, ' ') << shapes.size() << " shapes; spanning ";
        low_corner.print(false);
        cout << " to ";
        high_corner.print(false);
        cout << "; surface area = ";
        Vec3 tmp = high_corner - low_corner;
        cout << (2 * (tmp.x * tmp.y + tmp.y * tmp.z + tmp.z * tmp.x)) << endl;
    }
    else{
        cout << string(depth, ' ') << "internal node split at " << partition_distance << " over " << axis << endl;
        children[LEFT]->print(depth + 1);
        children[RIGHT]->print(depth + 1);
    }
}

void KDNode::collide(const Ray& r, Collision &c) const{
    // kd_recurses++;

    if (axis == LEAF){
        Collision temp_collision;
        // objects_checked += shapes.size();
        for (vector<Shape*>::const_iterator s_iter = shapes.begin(); s_iter != shapes.end(); ++s_iter){
            temp_collision = (*s_iter)->collide(r);
            if (temp_collision.collided && (temp_collision.distance < c.distance || !c.collided)){
                c = temp_collision;
            }
        }
    }
    else{
        // There is a collision with the splitting plane anywhere.
        if (r.direction[axis] != 0){
            // Optimized/simplified ray-plane intersection math.
            float t = (partition_distance - r.origin[axis]) / r.direction[axis];
            if (t < 0){
                // Ray is facing away from splitting plane and so only hits one child.
                collideOneChild(r, c);
                return;
            }
            Vec3 point = r.pointAt(t);
            if (low_corner < point && point < high_corner){
                // The ray hit the splitting plane, so it must hit both children!
                children[LEFT]->collide(r, c);
                children[RIGHT]->collide(r, c);
                
                /*
                // Check which child is the near and check it first.
                int near_index = r.origin[axis] < point[axis] ? LEFT : RIGHT;
                Collision near_collision;
                children[near_index]->collide(r, near_collision);
                if (near_collision.collided && (!c.collided || near_collision.distance < c.distance)){
                    c = near_collision;
                }
                else{
                    children[1 - near_index]->collide(r, c);
                }
                */
                
            }
            else{
                // The ray missed the plane even though it was facing it, but it still
                // hits one child.
                collideOneChild(r, c);
            }
        }
        // The ray only passes through one child.
        else{
            // Since the ray is parallel to the splitting plane, we can shortcut the ray-box
            // intersection: the ray will stay on whatever side it is on right now.
            if (r.origin[axis] < partition_distance){
                children[LEFT]->collide(r, c);
            }
            else{
                children[RIGHT]->collide(r, c);
            }
        }
    }
}

bool KDNode::collideBoolean(const Ray& r, const float d) const{
    // kd_recurses++;

    if (axis == LEAF){
        Collision temp_collision;
        // objects_checked += shapes.size();
        for (vector<Shape*>::const_iterator s_iter = shapes.begin(); s_iter != shapes.end(); ++s_iter){
            temp_collision = (*s_iter)->collide(r);
            if (temp_collision.collided && temp_collision.distance < d){
                return true;
            }
        }
        return false;
    }
    else{
        if (r.direction[axis] != 0){
            float t = (partition_distance - r.origin[axis]) / r.direction[axis];
            if (t < 0){
                return collideOneChildBoolean(r, d);
            }
            Vec3 point = r.pointAt(t);
            if (low_corner < point && point < high_corner){
                if (r.origin[axis] < point[axis]){
                    return children[LEFT]->collideBoolean(r, d) || children[RIGHT]->collideBoolean(r, d);
                }
                return children[RIGHT]->collideBoolean(r, d) || children[LEFT]->collideBoolean(r, d);
            }
            else{
                return collideOneChildBoolean(r, d);
            }
        }
        else{
            if (r.origin[axis] < partition_distance){
                return children[LEFT]->collideBoolean(r, d);
            }
            else{
                return children[RIGHT]->collideBoolean(r, d);
            }
        }
    }
}

// Perform ray-box intersection tests to figure out which of the two children this ray hits.
// See rectPrism::collide() for an explanation of this function.
void KDNode::collideOneChild(const Ray& r, Collision &c) const{
    Vec3 high_mid_corner(high_corner);
    high_mid_corner[axis] = partition_distance;
    for (int corner = 0; corner < 2; ++corner){
        for (int i = 0; i < 3; ++i){
            if (r.direction[i] != 0){
                float t = (corner == 0 ? high_mid_corner[i] - r.origin[i] : low_corner[i] - r.origin[i]) / r.direction[i];
                if (t < 0){
                    continue;
                }
                Vec3 point = r.pointAt(t);
                point[i] -= (1 - 2 * corner) * EPSILON;
                if (low_corner <= point && point <= high_mid_corner){
                    // Collision with the left child.
                    children[LEFT]->collide(r, c);
                    return;
                }
            }
        }
    }
    children[RIGHT]->collide(r, c);
}

bool KDNode::collideOneChildBoolean(const Ray& r, const float d) const{
    Vec3 high_mid_corner(high_corner);
    high_mid_corner[axis] = partition_distance;
    for (int corner = 0; corner < 2; ++corner){
        for (int i = 0; i < 3; ++i){
            if (r.direction[i] != 0){
                float t = (corner == 0 ? high_mid_corner[i] - r.origin[i] : low_corner[i] - r.origin[i]) / r.direction[i];
                if (t < 0){
                    continue;
                }
                Vec3 point = r.pointAt(t);
                point[i] -= (1 - 2 * corner) * EPSILON;
                if (low_corner <= point && point <= high_mid_corner){
                    // Collision with the left child.
                    return children[LEFT]->collideBoolean(r, d);
                }
            }
        }
    }
    return children[RIGHT]->collideBoolean(r, d);
}

void KDNode::split(){
    children[LEFT] = children[RIGHT] = NULL;
    partition_distance = 0;
    if (shapes.size() < KD_SPLIT_THRESHOLD){
        axis = LEAF;
        return;
    }

    Vec3 size = high_corner - low_corner;
    float inv_surface_area_current = 0.5f / (size.x * size.y + size.y * size.z + size.z * size.x);

    float best_partition_distances[3] = {0, 0, 0};
    float best_costs[3] = {-1, -1, -1};
    for (axis = 0; axis < 3; ++axis){
        // Create the unique set of potential partitions that will be checked.
        set<float> partition_set;
        for (vector<Shape*>::iterator s_iter = shapes.begin(); s_iter != shapes.end(); ++s_iter){
            partition_set.insert((*s_iter)->extremeValue(axis, EXTREME_VALUE_SMALLEST));
            partition_set.insert((*s_iter)->extremeValue(axis, EXTREME_VALUE_LARGEST));
        }

        // Initialize the PartitionInfo structs.
        float low_bound = low_corner[axis], high_bound = high_corner[axis];
        vector<PartitionInfo> partitions;
        for (set<float>::iterator p_iter = partition_set.begin(); p_iter != partition_set.end(); ++p_iter){
            float current_partition_distance = *p_iter;
            // Cut out any partitions that are beyond the range of this box.
            if (current_partition_distance > low_bound && current_partition_distance < high_bound){
                partitions.push_back(PartitionInfo(low_corner, high_corner, axis, current_partition_distance));
            }
        }
        
        // Not sure when this actually happens, but it does...
        if (partitions.empty()){
            continue;
        }

        // Then sort them, quite literally, left to right to make the next step work.
        sort(partitions.begin(), partitions.end());

        // For each shape, make sure all the partitions know if it's on the right or left (or straddles and is on both).
        int num_partitions = partitions.size();
        for (vector<Shape*>::iterator s_iter = shapes.begin(); s_iter != shapes.end(); ++s_iter){
            int last_left_index = binarySearchPartitions(partitions, (*s_iter)->extremeValue(axis, EXTREME_VALUE_SMALLEST)),
                first_right_index = binarySearchPartitions(partitions, (*s_iter)->extremeValue(axis, EXTREME_VALUE_LARGEST));
            for (int i = 0; i <= last_left_index; ++i){
                partitions[i].count_right++;
            }
            for (int i = first_right_index; i < num_partitions; ++i){
                partitions[i].count_left++;
            }
            for (int i = last_left_index + 1; i < first_right_index; ++i){
                partitions[i].count_left++;
                partitions[i].count_right++; 
            }
        }

        // Find the best partition to use using the surface area heuristic.
        for (vector<PartitionInfo>::iterator p_iter = partitions.begin(); p_iter != partitions.end(); ++p_iter){
            PartitionInfo candidate = *p_iter;
            // 1.5 is traversal cost, 1.0 is the cost of doing an intersection. Can be tweaked.
            float cost = 2.5f + 0.9f * (candidate.surface_area_left * candidate.count_left * inv_surface_area_current +
                                         candidate.surface_area_right * candidate.count_right * inv_surface_area_current);
            if (best_costs[axis] == -1 || cost < best_costs[axis]){
                best_partition_distances[axis] = candidate.partition_distance;
                best_costs[axis] = cost;
            }
        }

        // And make sure it's worth using: this is the baseline cost the partitioning schemes must beat.
        if (best_costs[axis] > 1.0f * shapes.size()){
            best_costs[axis] = -1;
            continue;
        }
    }

    // Figure out the axis with the best score.
    axis = LEAF;
    for (int i = 0; i < 3; ++i){
        if (best_costs[i] != -1 && (axis == LEAF || best_costs[i] < best_costs[axis])){
            axis = i;
        }
    }
    if (axis == LEAF){
        return;
    }

    // Member variable.
    partition_distance = best_partition_distances[axis];
    
    Vec3 high_mid_corner = high_corner, low_mid_corner = low_corner;
    high_mid_corner[axis] = low_mid_corner[axis] = partition_distance;

    // Assign all this node's shapes to either or both children.
    vector<Shape*> left_shapes, right_shapes;
    for (vector<Shape*>::iterator s_iter = shapes.begin(); s_iter != shapes.end(); ++s_iter){
        Shape *s = *s_iter;
        // if (s->collidesWithBox(low_corner, high_mid_corner)){
        if (s->extremeValue(axis, EXTREME_VALUE_SMALLEST) < partition_distance){
            left_shapes.push_back(s);
        }
        // if (s->collidesWithBox(low_mid_corner, high_corner)){
        if (s->extremeValue(axis, EXTREME_VALUE_LARGEST) > partition_distance){
            right_shapes.push_back(s);
        }
    }
    shapes.clear();
    
    children[LEFT] = new KDNode(left_shapes, low_corner, high_mid_corner);
    children[RIGHT] = new KDNode(right_shapes, low_mid_corner, high_corner);
}

void KDNode::calculateAggregateBounds(){
    low_corner =  Vec3( numeric_limits<float>::infinity(),  numeric_limits<float>::infinity(),  numeric_limits<float>::infinity());
    high_corner = Vec3(-numeric_limits<float>::infinity(), -numeric_limits<float>::infinity(), -numeric_limits<float>::infinity());
    vector<Shape*>::const_iterator iter;
    for (iter = shapes.begin(); iter != shapes.end(); ++iter){
        Shape *s = *iter;
        for (int i = 0; i < 3; ++i){
            if (s->extremeValue(i, EXTREME_VALUE_SMALLEST) < low_corner[i]){
                low_corner[i] = s->extremeValue(i, EXTREME_VALUE_SMALLEST);
            }
            if (s->extremeValue(i, EXTREME_VALUE_LARGEST) > high_corner[i]){
                high_corner[i] = s->extremeValue(i, EXTREME_VALUE_LARGEST);
            }
        }
    }
}
