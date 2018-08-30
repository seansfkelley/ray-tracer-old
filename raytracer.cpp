#include "raytracer.h"

#include <cmath>
#include <algorithm>
#include <set>

#include "collision.h"

Raytracer::Raytracer(const Vec3 &eye, const Vec3 &grid_center, float rotation_degrees,
                     int resx, int resy, float scaling_factor, int antialias_samples,
                     int num_photons,
                     const Color &background, const Color &ambient, 
                     const vector<Light> &lights, const vector<Shape*> &shapes){

    // Z: Looks from target towards camera.
    Vec3 cam_z_vec = (grid_center - eye).asNormal();

    // Y: Cross Z with Z dropped onto the XZ plane and rotated 90 degrees CCW (when looking down the world Y-axis).
    cam_y_vec = cam_z_vec.cross(Vec3(cam_z_vec.z, 0, -cam_z_vec.x)).asNormal();

    // Y is now in its up (unrotated) position. Rotate it in the plane defined by the Z vector as the normal,
    // going clockwise by expanding out the rotation matrix into a series of individual operations.
	float cos_theta = cos(rotation_degrees * DEG_TO_RAD), sin_theta = sin(rotation_degrees * DEG_TO_RAD);
    float a_x = cam_z_vec.x, a_y = cam_z_vec.y, a_z = cam_z_vec.z;
    Vec3 rotated_cam_y_vec;
    rotated_cam_y_vec.x = (cos_theta + (1 - cos_theta) * a_x * a_x) * cam_y_vec.x + 
                          ((1 - cos_theta) * a_x * a_y - a_z * sin_theta) * cam_y_vec.y + 
                          ((1 - cos_theta) * a_x * a_z + a_y * sin_theta) * cam_y_vec.z;

	rotated_cam_y_vec.y = ((1 - cos_theta) * a_x * a_y + a_z * sin_theta) * cam_y_vec.x + 
                          (cos_theta + (1 - cos_theta) * a_y * a_y) * cam_y_vec.y + 
                          ((1 - cos_theta) * a_y * a_z - a_x * sin_theta) * cam_y_vec.z;

	rotated_cam_y_vec.z = ((1 - cos_theta) * a_x * a_z - a_y * sin_theta) * cam_y_vec.x + 
                          ((1 - cos_theta) * a_y * a_z + a_x * sin_theta) * cam_y_vec.y + 
                          (cos_theta + (1 - cos_theta) * a_z * a_z) * cam_y_vec.z;

    cam_y_vec = rotated_cam_y_vec * scaling_factor;
    cam_x_vec = cam_z_vec.cross(cam_y_vec).asNormal() * scaling_factor;

    origin = grid_center - (cam_x_vec * (resx / 2.0)) - (cam_y_vec * (resy / 2.0));

    this->eye = eye;
    this->ambient = ambient;
    bkrd = background;
    this->lights = lights;

    aa_samples = antialias_samples;
    this->resx = resx;
    this->resy = resy;

    cerr << "Building kd-tree (" << shapes.size() << " shapes)... ";
    cerr.flush();
    kdtree = KDNode(shapes);
    cerr << "done" << endl;
    
    if (num_photons != 0){
        using_photons = true;
        createPhotonMap(num_photons, lights);
    }
    else{
        using_photons = false;
    }
}

Color Raytracer::colorTrace(int x, int y, CRandomMersenne& twister) const{
    Ray r;
    if (aa_samples == 1){
        // 0.5 makes the ray go through the middle of the grid space.
        r.origin = origin + (cam_x_vec * (x + 0.5)) + (cam_y_vec * (y + 0.5));
        r.direction = r.origin - eye;
        r.direction.normalize();

        return colorTrace(r, twister);
    }
    
    Color total_color(0, 0, 0);
    for (int i = 0; i < aa_samples; ++i){
        for (int j = 0; j < aa_samples; ++j){
            // Sample randomly, but make sure each subpixel grid square gets representation.
            r.origin = origin + (cam_x_vec * (x + (i + (float) twister.Random()) / aa_samples))
                              + (cam_y_vec * (y + (j + (float) twister.Random()) / aa_samples));
            r.direction = r.origin - eye;
            r.direction.normalize();
            
            total_color += colorTrace(r, twister);
        }
    }
    return total_color / (aa_samples * aa_samples);
}

Color Raytracer::colorTrace(const Ray &r, CRandomMersenne& twister, int depth) const{
    if (depth == MAX_REFLECTIONS){
        return bkrd;
    }

    Collision closest;
    kdtree.collide(r, closest);

    if (RENDER_PHOTON_MAP_ONLY){
        if (!closest.collided){
            return Color();
        }
        
        Vec3 collision_point = r.pointAt(closest.distance);

        vector<Photon*> photons;
        
        global_map.kNearestNeighbors(collision_point, 1, photons);
        Color c_photons_global;
        if (photons.size() > 0 && (photons[0]->point - collision_point).magnitude2() < 2){
            return photons[0]->color;
        }
        
        return Color();
    }
    
    Collision closest_light;
    closest_light.distance = 0; // Shutup, compiler.
    vector<Light>::const_iterator light_iter;
    for (light_iter = lights.begin(); light_iter != lights.end(); ++light_iter){
        Light l = *light_iter;
        float t = l.collide(r);
        if (t > 0 && (t < closest_light.distance || !closest_light.collided)){
            closest_light.collided = true;
            closest_light.distance = t;
            // Watch it -- this is a kind of hacky solution, but I wanted to contain all in the collision information
            // in one place. Making Lights into Shapes was out of the question. This hack will never pass beyond this
            // loop and the if statement directly following it. Promise. So don't worry.
            closest_light.normal = l.color;
        }
    }
    if (closest_light.collided && (closest_light.distance < closest.distance || !closest.collided)){
        return closest_light.normal;
    }

    if (closest.collided){
        Shape const *s = closest.shape;

        // Move outside the shape by EPSILON distance to fix rounding errors.
        Vec3 collision_point = r.pointAt(closest.distance) + (closest.normal * EPSILON);

        Color c_intrinsic = ambient * s->mat.color;
        // Only shade calculation for each source.
        for (light_iter = lights.begin(); light_iter != lights.end(); ++light_iter){
            Light l = *light_iter;
            float shade = 0;
            vector<Vec3> points = l.samplePoints(twister);
            vector<Vec3>::const_iterator point_iter;
            for (point_iter = points.begin(); point_iter != points.end(); ++point_iter){
                if (!booleanTrace(collision_point, *point_iter)){
                    ++shade;
                }
            }

            if (shade == 0){
                continue;
            }
            
            shade /= points.size();

            Vec3 collision_to_light_direction = l.pos - collision_point;
            float falloff = min<float>(1, FALLOFF / collision_to_light_direction.magnitude2()) * shade;
            collision_to_light_direction.normalize();
                    
            float diffuse = closest.normal.dot(collision_to_light_direction);
            if (diffuse > 0){
                c_intrinsic += (s->mat.color * l.color) * (s->mat.k_diffuse * diffuse * falloff);
            }

            if (false && s->mat.k_specular > 0){
                Vec3 light_refl_direction = collision_to_light_direction - (closest.normal * 2 * closest.normal.dot(collision_to_light_direction));
                float specular = pow(light_refl_direction.dot((r.origin - collision_point).asNormal()), s->mat.shininess);
                c_intrinsic += (s->mat.color * l.color) * (s->mat.k_specular * specular * falloff);
            }
        }

        Color c_reflected;
        if (s->mat.pct_refl > 0){
            Ray r_reflected;
            r_reflected.origin = collision_point;
            r_reflected.direction = r.direction - (closest.normal * 2 * closest.normal.dot(r.direction));
            c_reflected = colorTrace(r_reflected, twister, depth + 1);
        }

        Color c_refracted;
        // Only works if no two refractive objects intersect in any way.
        if (s->mat.pct_refr > 0){
            Ray r_refracted;
            // Move EPSILON distance back inside of the object, accounting for the previous
            // move of EPSILON distance in the declaration of collision_point.
            r_refracted.origin = collision_point - (closest.normal * EPSILON * 2);

            float n, n1, n2;
            if (r.inside_shape == NULL){
                n1 = 1; // 1 ~ IOR of air.
                n2 = s->mat.refr_index;
                n = n1 / n2;
                r_refracted.inside_shape = s;
            }
            else{
                n1 = s->mat.refr_index; // 1 ~ IOR of air.
                n2 = 1;
                n = n1; // Implied division by 1 (~ IOR of air).
                r_refracted.inside_shape = NULL;
            }

            float cos_i = r.direction.dot(closest.normal); // Why does some code use abs() of this?
            float sin_i2 = n * n * (1 - cos_i * cos_i);
            if (sin_i2 > 1){
                // Total internal reflection. Subtle difference between this and regular reflection: this
                // one's origin is inside the object. This is why we don't modify r_refracted.origin.
                r_refracted.direction = r.direction - (closest.normal * 2 * closest.normal.dot(r.direction));
                
                c_refracted = colorTrace(r_refracted, twister, depth + 1);
            }
            else{
                // Apply Fresnel's equations.
                r_refracted.direction = r.direction * n - closest.normal * (n * cos_i + sqrt(1 - sin_i2));

                float cos_t = r_refracted.direction.dot(closest.normal);
                float r_par =  (n2 * cos_i - n1 * cos_t) / (n2 * cos_i + n1 * cos_t);
                float r_perp = (n1 * cos_i - n2 * cos_t) / (n1 * cos_i + n2 * cos_t);
                float pct_reflected = 0.5 * (r_par * r_par + r_perp * r_perp);
                
                // For performance reasons, ignore the effect of Fresnel if it has a negligible impact.
                if (pct_reflected < FRESNEL_REFLECTIVE_MIN){
                    c_refracted = colorTrace(r_refracted, twister, depth + 1);
                }
                else{
                    Ray r_reflected;
                    r_reflected.origin = collision_point;
                    r_reflected.direction = r.direction - (closest.normal * 2 * closest.normal.dot(r.direction));

                    c_refracted = colorTrace(r_refracted, twister, depth + 1) * (1 - pct_reflected) + 
                                  colorTrace(r_reflected, twister, depth + 1) * pct_reflected;
                }
            }
        }

        Color c_photons;
        if (using_photons){
            /*
            Ray radiance_ray;
            radiance_ray.origin = collision_point;
            for (int i = 0; i < 10; ++i){
                radiance_ray.direction = r.direction - (closest.normal * 2 * closest.normal.dot(r.direction));

                Vec3 x_dir(radiance_ray.direction.z, radiance_ray.direction.y, -radiance_ray.direction.x);
                Vec3 y_dir = radiance_ray.direction.cross(x_dir);

                float x_offset, y_offset;
                do{
                    x_offset = 2 * (twister.Random() - 0.5) * 0.25;
                    y_offset = 2 * (twister.Random() - 0.5) * 0.25;
                }
                while ((x_offset * x_offset + y_offset * y_offset) > (0.25 * 0.25));

                radiance_ray.direction += x_dir * x_offset + y_dir * y_offset;
                radiance_ray.direction.normalize();

                c_photons += radianceTrace(radiance_ray);
            }

            c_photons /= 10;*/

            
            vector<Photon*> photons;
            global_map.kNearestNeighbors(collision_point, K_NEAREST_AMT, photons);

            if (photons.size() > 0){
                Color c_photons_global;
                for (vector<Photon*>::iterator p_iter = photons.begin(); p_iter != photons.end(); ++p_iter){
                    Photon *p = *p_iter;
                    float diffuse = closest.normal.dot(-(p->incident_direction));
                    if (diffuse > 0){
                        c_photons_global += (s->mat.color * p->color) * (s->mat.k_diffuse * diffuse);
                    }
                }
                
                c_photons_global /= (PI * (photons[0]->point - collision_point).magnitude2());
                
                c_photons += c_photons_global;
            }
            
            caustics_map.kNearestNeighbors(collision_point, K_NEAREST_AMT, photons);

            if (photons.size() > 0){
                Color c_photons_caustic;
                for (vector<Photon*>::iterator p_iter = photons.begin(); p_iter != photons.end(); ++p_iter){
                    c_photons_caustic += (*p_iter)->color;
                }
                
                c_photons_caustic /= (PI * (photons[0]->point - collision_point).magnitude2());
                
                c_photons += c_photons_caustic;
            }
        }

        // Since the lighting model allows the values to be outside [0, 1] (more frequently being too high than too low, if that ever even happens),
        // we have to clamp the value to the allowable range before it leaves this function. All other code should expect valid values.
        return (c_intrinsic * (1 - s->mat.pct_refl - s->mat.pct_refr) + c_reflected * s->mat.pct_refl + c_refracted * s->mat.pct_refr + c_photons).asClamped0_1();
    }

    return bkrd;
}

bool Raytracer::booleanTrace(const Vec3 &from, const Vec3 &to) const{
    Ray r;
    r.origin = from;
    r.direction = to - from;
    float dist = r.direction.magnitude();
    r.direction.normalize();

    return kdtree.collideBoolean(r, dist);
}

Color Raytracer::radianceTrace(const Ray &r) const{
    Collision closest;
    kdtree.collide(r, closest);

    if (closest.collided){
        Vec3 collision_point = r.pointAt(closest.distance) + (closest.normal * EPSILON);

        vector<Photon*> photons;
        global_map.kNearestNeighbors(collision_point, K_NEAREST_AMT, photons);
        
        Color radiance;
        for (vector<Photon*>::iterator p_iter = photons.begin(); p_iter != photons.end(); ++p_iter){
            radiance += (*p_iter)->color * max<float>(closest.normal.dot(-((*p_iter)->incident_direction)), 0);
        }
        return radiance / (PI * (photons[0]->point - collision_point).magnitude2());
    }
    else{
        return Color();
    }
}

void Raytracer::photonTrace(const Color &color, const Ray &r, CRandomMersenne &twister, int depth, bool is_caustic, vector<Photon> &global, vector<Photon> &caustics) const{
    if (depth == MAX_REFLECTIONS){
            return;
    }

    Collision closest;
    kdtree.collide(r, closest);

    if (closest.collided){
        Shape const *s = closest.shape;
        Color new_color = color * s->mat.color;

        // Move outside the shape by EPSILON distance to fix rounding errors.
        Vec3 collision_point = r.pointAt(closest.distance) + (closest.normal * EPSILON);
        
        float random_var = twister.Random();
        if (random_var < s->mat.pct_refl){
            Ray r_reflected;
            r_reflected.origin = collision_point;
            r_reflected.direction = r.direction - (closest.normal * 2 * closest.normal.dot(r.direction));
            photonTrace(new_color, r_reflected, twister, depth + 1, false, global, caustics);
        }
        else if (random_var < s->mat.pct_refl + s->mat.pct_refr){
            Ray r_refracted;
            // Move EPSILON distance back inside of the object, accounting for the previous
            // move of EPSILON distance in the declaration of collision_point.
            r_refracted.origin = collision_point - (closest.normal * EPSILON * 2);

            float n, n1, n2;
            if (r.inside_shape == NULL){
                n1 = 1; // 1 ~ IOR of air.
                n2 = s->mat.refr_index;
                n = n1 / n2;
                r_refracted.inside_shape = s;
            }
            else{
                n1 = s->mat.refr_index; // 1 ~ IOR of air.
                n2 = 1;
                n = n1; // Implied division by 1 (~ IOR of air).
                r_refracted.inside_shape = NULL;
            }

            float cos_i = r.direction.dot(closest.normal); // Why does some code use abs() of this?
            float sin_i2 = n * n * (1 - cos_i * cos_i);
            if (sin_i2 > 1){
                // Total internal reflection. Subtle difference between this and regular reflection: this
                // one's origin is inside the object. This is why we don't modify r_refracted.origin.
                r_refracted.direction = r.direction - (closest.normal * 2 * closest.normal.dot(r.direction));
                
                photonTrace(new_color, r_refracted, twister, depth + 1, false, global, caustics);
            }
            else{
                // Apply Fresnel's equations.
                r_refracted.direction = r.direction * n - closest.normal * (n * cos_i + sqrt(1 - sin_i2));

                float cos_t = r_refracted.direction.dot(closest.normal);
                float r_par =  (n2 * cos_i - n1 * cos_t) / (n2 * cos_i + n1 * cos_t);
                float r_perp = (n1 * cos_i - n2 * cos_t) / (n1 * cos_i + n2 * cos_t);
                float pct_reflected = 0.5 * (r_par * r_par + r_perp * r_perp);
                
                // For performance reasons, ignore the effect of Fresnel if it has a negligible impact.
                if (pct_reflected < FRESNEL_REFLECTIVE_MIN){
                    photonTrace(new_color, r_refracted, twister, depth + 1, is_caustic, global, caustics);
                }
                else{
                    Ray r_reflected;
                    r_reflected.origin = collision_point;
                    r_reflected.direction = r.direction - (closest.normal * 2 * closest.normal.dot(r.direction));

                    photonTrace(new_color, r_refracted, twister, depth + 1, true, global, caustics);
                    photonTrace(new_color, r_reflected, twister, depth + 1, true, global, caustics);
                }
            }
        }
        else if (twister.Random() < s->mat.k_diffuse){
            if (depth > 0){
                Photon p;
                p.point = collision_point;
                p.incident_direction = r.direction;
                p.normal = closest.normal;
                p.color = color;
                if (is_caustic){
                    caustics.push_back(p);
                }
                else{
                    global.push_back(p);
                }
            }
            
            if (!is_caustic){
                Ray r_reflected;
                r_reflected.origin = collision_point;
                r_reflected.direction = r.direction - (closest.normal * 2 * closest.normal.dot(r.direction));
                photonTrace(new_color * s->mat.k_diffuse, r_reflected, twister, depth + 1, false, global, caustics);
            }
        }
        else{
            if (depth > 0){
                Photon p;
                p.point = collision_point;
                p.incident_direction = r.direction;
                p.normal = closest.normal;
                p.color = color;
                if (is_caustic){
                    caustics.push_back(p);
                }
                else{
                    global.push_back(p);
                }
            }
        }
    }
}

void Raytracer::createPhotonMap(int num_photons, const vector<Light> &lights){
    CRandomMersenne twister(time(NULL));
    num_photons /= lights.size();
    
    vector<Photon> global, caustics;

    cerr << "Firing " << (num_photons * lights.size()) << " photons... ";
    cerr.flush();
    for (vector<Light>::const_iterator l_iter = lights.begin(); l_iter != lights.end(); ++l_iter){
        Light l = *l_iter;
        for (int i = 0; i < num_photons; ++i){
            photonTrace(l.color, 
                        Ray(l.pos, Vec3(twister.Random() - 0.5, twister.Random() - 0.5, twister.Random() - 0.5).asNormal()),
                        twister,
                        0,
                        false,
                        global,
                        caustics);
        }
    }
    cerr << "done" << endl;

    Photon *photon_array = new Photon[global.size()];
    copy(global.begin(), global.end(), photon_array);
    for (unsigned int i = 0; i < global.size(); ++i){
        photon_array[i].color /= global.size() * GLOBAL_POWER_SCALING;
    }
   
    cerr << "Building global photon map from resulting " << global.size() << " photons... ";
    cerr.flush();
    global_map = PhotonMap(photon_array, global.size());
    cerr << "done" << endl;

    photon_array = new Photon[caustics.size()];
    copy(caustics.begin(), caustics.end(), photon_array);
    for (unsigned int i = 0; i < caustics.size(); ++i){
        photon_array[i].color /= caustics.size() * CAUSTICS_POWER_SCALING;
    }
   
    cerr << "Building caustics photon map from resulting " << caustics.size() << " photons... ";
    cerr.flush();
    caustics_map = PhotonMap(photon_array, caustics.size());
    cerr << "done" << endl;
}

int Raytracer::getX() const{
    return resx;
}

int Raytracer::getY() const{
    return resy;
}

int Raytracer::getAASamples() const{
    return aa_samples;
}
