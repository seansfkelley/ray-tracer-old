#include "processinput.h"

#include <cassert>
#include <unistd.h>

#include "shapes.h"

void checkMaterialName(map<string, Material> &materials, char *name){
    if (materials.find(name) == materials.end()){
        cerr << "Reference to unknown material \'" << name << "\'." << endl;
        exit(EXIT_FAILURE);
    }
}

void checkFloat0_1(float value, string value_name = ""){
    if (value < 0 || value > 1){
        cerr << "Value error: ";
        if (!value_name.compare("")){
            cerr << "value";
        }
        else{
            cerr << value_name;
        }
        cerr << " out of allowable range [0, 1]." << endl;
        exit(EXIT_FAILURE);
    }
}

void checkVector0_1(Vec3 &vector, string vector_name = ""){
    // This is NOT equivalent to vector < Vec3() || vector > Vec3(1, 1, 1) because of the way the comparison operators are overloaded.
    if (!(vector >= Vec3() && vector <= Vec3(1, 1, 1))){
        cerr << "Value error: ";
        if (!vector_name.compare("")){
            cerr << "triplet";
        }
        else{
            cerr << vector_name;
        }
        cerr << " out of allowable range [0, 1] for each component." << endl;
        exit(EXIT_FAILURE);
    }
}

void checkFloatIsPositive(float value, string value_name = ""){
    if (value <= 0){
        cerr << "Value error: ";
        if (!value_name.compare("")){
            cerr << "value";
        }
        else{
            cerr << value_name;
        }
        cerr << " is not positive." << endl;
        exit(EXIT_FAILURE);
    }
}

void goToTag(istream &input, string tag){
    string line;
    do{
        getline(input, line);
    } while (!input.eof() && line.compare(tag));

    if (input.eof() && line.compare(tag)){
        cout << "Missing " << tag << " tag. Check input file." << endl;
        exit(EXIT_FAILURE);
    }
}

Raytracer processInput(istream &input){
    input.exceptions(istream::failbit | istream::badbit);

    try{
        cout << "Parsing input file... ";
        cout.flush();

        // Begin parsing parameters.
        goToTag(input, "#parameters");
        int resx, resy;
        input >> resx >> resy;
        checkFloatIsPositive(resx, "image width");
        checkFloatIsPositive(resy, "image height");

        float scaling_factor;
        input >> scaling_factor;
        checkFloatIsPositive(scaling_factor, "scaling factor");

        int antialias_samples;
        input >> antialias_samples;
        checkFloatIsPositive(antialias_samples, "antialias samples");

        float eyex, eyey, eyez;
        input >> eyex >> eyey >> eyez;
    
        float focusx, focusy, focusz;
        input >> focusx >> focusy >> focusz;

        float focal_length;
        input >> focal_length;
        checkFloatIsPositive(focal_length, "focal length");

        float rotation_degrees;
        input >> rotation_degrees;

        int photons;
        input >> photons;
        // Add 1, because we want zero to be an acceptable value.
        checkFloatIsPositive(photons + 1, "number of photons");

        float ar, ag, ab;
        input >> ar >> ag >> ab;

        float br, bg, bb;
        input >> br >> bg >> bb;

        Vec3 eye(eyex, eyey, eyez), focus (focusx, focusy, focusz);
        Vec3 grid_center = Vec3((focus - eye).asNormal() * focal_length) + eye;
        Color ambient(ar, ag, ab), background(br, bg, bb);

        checkVector0_1(ambient, "ambient light values");
        checkVector0_1(background, "background light values");

        // Begin parsing lights.
        goToTag(input, "#lights");
        int num_lights;
        input >> num_lights;

        vector<Light> lights;
        for (; num_lights > 0; num_lights--){
            Light l;
            float x, y, z;
            input >> x >> y >> z;
            l.pos = Vec3(x, y, z);

            float side_length;
            input >> side_length;
            int samples;
            input >> samples;
            checkFloatIsPositive(samples, "samples for light");
            if (samples > 1){
                checkFloatIsPositive(side_length, "more than one sample desired and side length");
            }
            else if (side_length != 0){
                cerr << "Warning: side length for light specified, but only one sample used: treating as point light source." << endl;
            }
            l.setArea(side_length, samples);

            float r, g, b;
            input >> r >> g >> b;
            l.color = Color(r, g, b);
            checkVector0_1(l.color, "color of light");

            lights.push_back(l);
        }

        // Begin parsing materials.
        goToTag(input, "#materials");
        int num_materials;
        input >> num_materials;

        map<string, Material> materials;
        for (; num_materials > 0; num_materials--){
            Material m;
            string material_name;
            float r, g, b;
            input >> material_name >> r >> g >> b;
            m.color = Color(r, g, b);
            checkVector0_1(m.color, "material color values");
        
            float diff, spec;
            input >> diff >> spec;
            checkFloat0_1(diff, "material diffuse value");
            checkFloat0_1(spec, "material specular value");
            checkFloat0_1(diff + spec, "material total diffuse and specular value");
            m.k_diffuse = diff;
            m.k_specular = spec;

            float refl, refr;
            input >> refl >> refr;
            checkFloat0_1(refl, "material reflective value");
            checkFloat0_1(refr, "material refractive value");
            checkFloat0_1(refl + refr, "sum of reflective and refractive values");
            m.pct_refl = refl;
            m.pct_refr = refr;

            float shiny, ior;
            input >> shiny >> ior;
            if (spec > 0){
                checkFloatIsPositive(shiny, "specular reflection set and shininess");
            }
            if (refr > 0){
                checkFloatIsPositive(ior, "refraction set and index of refraction");
            }
            m.shininess = shiny;
            m.refr_index = ior;

            if (materials.find(material_name) != materials.end()){
                cerr << "Warning: redefinition of material \'" << material_name << "\'." << endl;
            }
            materials[material_name] = m;
        }

        // Begin parsing shapes.
        goToTag(input, "#shapes");
        int num_shapes;
        input >> num_shapes;

        vector<Shape*> shapes;
        for (; num_shapes > 0; num_shapes--){
            // Skip blank lines.
            string line, shape_name;
            do{
                getline(input, line);
            } while (line.find_first_not_of(" \t\n") == string::npos);

            shape_name = line.substr(0, line.find_first_of(" \t"));

            char material[128];
            float x, y, z;
            if (!shape_name.compare("sphere")){
                float r;
                sscanf(line.c_str(), "sphere %s %f %f %f %f\n", material, &x, &y, &z, &r);
                checkMaterialName(materials, material);
                checkFloatIsPositive(r, "sphere radius");
                Sphere *s = new Sphere(materials[material], Vec3(x, y, z), r);
                shapes.push_back(dynamic_cast<Shape*>(s));
            }
            else if (!shape_name.compare("rectprism")){
                float dx, dy, dz;
                sscanf(line.c_str(), "rectprism %s %f %f %f %f %f %f\n", material, &x, &y, &z, &dx, &dy, &dz);
                checkMaterialName(materials, material);
                RectPrism *rp = new RectPrism(materials[material], Vec3(x, y, z), Vec3(dx, dy, dz));
                shapes.push_back(dynamic_cast<Shape*>(rp));
            }
        }
        cout << "done" << endl; // "Parsing input file... "

        return Raytracer(eye, grid_center, rotation_degrees, resx, resy, scaling_factor, antialias_samples, photons, background, ambient, lights, shapes);
    }
    catch (istream::failure f){
        cerr << "Syntactical error while reading from input." << endl;
        exit(EXIT_FAILURE);
    }

    assert(false);
    return Raytracer();
}

int processArguments(int argc, char **argv, string &host_ip, string &port, string &filename, uint8_t &num_threads){
    opterr = 0;
    int program_type = LOCAL;
    bool s_c_option_set = false;

    while (true){
        int i = getopt(argc, argv, ":sc:p:t:");
        if (i == -1){
            break;
        }
        char c = i;

        if (c == '?'){
            cerr << "Ignoring unknown option \'" << optopt << "\'" << endl;
            continue;
        }
        if (c == ':'){
            switch (optopt){
            case 'c':
                cerr << "Missing required server IP for -c option." << endl;
                break;

            case 'p':
                cerr << "Missing required port number for -p option." << endl;
                break;

            case 't':
                cerr << "Missing required thread count argument for -t option." << endl;
                break;

            default:
                assert(false);
            }
            exit(EXIT_FAILURE);
        }

        switch (c){
        case 's':
        case 'c':
            if (s_c_option_set){
                cerr << "Error: multiple -s or -c options specified." << endl;
                exit(EXIT_FAILURE);
            }
            s_c_option_set = true;
            if (c == 'c'){
                host_ip = optarg;
                program_type = CLIENT;
            }
            else{
                program_type = SERVER;
            }
            break;

        case 'p':
            port = optarg;
            break;

        case 't':
            // Enclose in a block because of initialization of local 'threads'.
            {
                int threads = atoi(optarg);
                if (threads < 1 || threads > 255){
                    cerr << "Invalid number of threads specified." << endl;
                    exit(EXIT_FAILURE);
                }
                num_threads = threads;
            }
            break;

        default:
            assert(false);
        }
    }

    if (program_type != CLIENT){
        if (optind < argc){
            filename = argv[optind];
        }
        else{
            cerr << "Input file required." << endl;
            exit(EXIT_FAILURE);
        }
    }

    return program_type;
}
