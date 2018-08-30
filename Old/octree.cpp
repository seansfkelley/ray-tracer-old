#include "octree.h"

#include <algorithm>
#include <iostream>

// long octree_recurses = 0;

OctreeNode::OctreeNode(const vector<Shape*> &s){
    is_split = false;
    shapes = s;
    centerOnAggregateBounds();
    split();
}

OctreeNode::OctreeNode(const Vec3 &corner_one, const Vec3 &corner_two, const vector<Shape*> &s){
    is_split = false;
    shapes = s;
    low_corner = Vec3(min<float>(corner_one.x, corner_two.x),
                      min<float>(corner_one.y, corner_two.y),
                      min<float>(corner_one.z, corner_two.z));
    high_corner = Vec3(max<float>(corner_one.x, corner_two.x),
                       max<float>(corner_one.y, corner_two.y),
                       max<float>(corner_one.z, corner_two.z));
    center = (low_corner + high_corner) / 2;
}

void OctreeNode::collide(const Ray &r, vector<Shape*> &collisions) const{
    // octree_recurses++;

    if (is_split){
        bool children_to_check[8] = {false};

        // Check 6 outer planes, just like a RectPrism.
        float num, den, dist;
        Vec3 point;
        for (int corner = 0; corner < 2; ++corner){
            for (int i = 0; i < 3; ++i){
                if (corner == 0){
                    num = high_corner[i] - r.origin[i];
                    den = r.direction[i];
                }
                else{
                    num = r.origin[i] - low_corner[i];
                    den = -r.direction[i];
                }

                //Handle infinite case?

                if (den != 0){
                    dist = num / den;
                    if (dist < 0){
                        continue;
                    }
                    point = r.pointAt(dist);
                    point[i] -= (1 - 2 * corner) * EPSILON;

                    if (low_corner <= point && point <= high_corner){
                        children_to_check[octantOfPoint(point)] = true;
                    }
                }
            }
        }

        // Check 3 inner planes. Here, two settings are flipped for each collision,
        // since the ray will leave one octant for the one on the other side.
        for (int i = 0; i < 3; ++i){
            num = center[i] - r.origin[i];
            den = r.direction[i];
            if (den != 0){
                dist = num / den;
                if (dist < 0){
                    continue;
                }
                point = r.pointAt(dist);
                if (low_corner <= point && point <= high_corner){
                    unsigned int octant = octantOfPoint(point);
                    children_to_check[octant] = true;
                    // Flip the bit of the octant corresponding to the plane being checked
                    // because the ray will hit the octants on both side of the plane.
                    children_to_check[octant ^ (((unsigned int) 1) << (2 - i))] = true;
                }
            }
        }

        for (int i = 0; i < 8; ++i){
            if (children_to_check[i]){
                children[i]->collide(r, collisions);
            }
        }

    }
    else{
        collisions.insert(collisions.end(), shapes.begin(), shapes.end());
    }
}

void OctreeNode::print(int depth) const{
    for (int i = 0; i < depth; ++i){
        cout << ' ';
    }
    cout << "OctreeNode(";
    low_corner.print(false);
    cout << ";";
    high_corner.print(false);
    cout << ";";
    if (is_split){
        cout << "split):" << endl;
        for (int i = 0; i < 8; ++i){
            children[i]->print(depth + 1);
        }
    }
    else{
        cout << shapes.size() << ")" << endl;
    }
}

// Generates a corner of an octree with the given bounds.
inline Vec3 generateCorner(const Vec3 &low, const Vec3 &high, unsigned int which){
    return Vec3(which & ((unsigned int) 4) ? high.x : low.x,
                which & ((unsigned int) 2) ? high.y : low.y,
                which & ((unsigned int) 1) ? high.z : low.z);
}

void OctreeNode::split(){
    if (shapes.size() > SPLIT_THRESHOLD){
        is_split = true;

        vector<Shape*> children_shapes[8];
        vector<Shape*>::const_iterator iter;
        for (iter = shapes.begin(); iter != shapes.end(); ++iter){
            shapeIntersections(*iter, children_shapes);
        }
        
        for (unsigned int i = 0; i < 8; ++i){
            children[i] = new OctreeNode(center, generateCorner(low_corner, high_corner, i), children_shapes[i]);
            children[i]->split();
        }
    }
    else{
        for (int i = 0; i < 8; ++i){
            children[i] = NULL;
        }
    }
}

void OctreeNode::shapeIntersections(Shape *s, vector<Shape*> *children_shapes){
    bool intersections[8] = {false};
    Vec3 corner;

    for (unsigned int i = 0; i < 8; ++i){
        corner = generateCorner(s->bounds_min, s->bounds_max, i);
        intersections[octantOfPoint(corner)] = true;
    }

    for (int i = 0; i < 8; ++i){
        if (intersections[i]){
            children_shapes[i].push_back(s);
        }
    }
}

unsigned int OctreeNode::octantOfPoint(const Vec3 &point) const{
    unsigned int octant = 0;

    if (point.x > center.x){
        octant += 4;
    }
    if (point.y > center.y){
        octant += 2;
    }
    if (point.z > center.z){
        octant += 1;
    }

    return octant;
}

void OctreeNode::centerOnAggregateBounds(){
    low_corner = shapes.front()->bounds_min, high_corner = shapes.front()->bounds_max;
    vector<Shape*>::const_iterator iter;
    for (iter = shapes.begin(); iter != shapes.end(); ++iter){
        Shape *s = *iter;
        for (int i = 0; i < 3; ++i){
            if (s->bounds_min[i] < low_corner[i]){
                low_corner[i] = s->bounds_min[i];
            }
            if (s->bounds_max[i] > high_corner[i]){
                high_corner[i] = s->bounds_max[i];
            }
        }
    }
    
    center = (low_corner + high_corner) / 2;
}

