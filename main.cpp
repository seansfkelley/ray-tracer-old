#include "constants.h"

#include <fstream>

#ifdef HAVE_PNGWRITER
#include <pngwriter.h>
#endif

#include <boost/thread/thread.hpp>

#include "network.h"
#include "server.h"
#include "client.h"
#include "raytracer.h"
#include "localworkerthread.h"
#include "processinput.h"

// extern long objects_checked, kd_recurses;

int main(int argc, char *argv[]){
    if (argc < 2){
        cout << "To raytrace locally: " << argv[0] << " <filename>" << endl;
        cout << "To run a server: " << argv[0] << " -s <filename>" << endl;
        cout << "To run a client: " << argv[0] << " -c <host>" << endl;
        cout << "If unsupplied, port defaults to " << DEFAULT_PORT << "." << endl;
        exit(EXIT_SUCCESS);
    }

    string host_ip, port = DEFAULT_PORT, filename;
    uint8_t num_threads = DEFAULT_THREADS;

    switch (processArguments(argc, argv, host_ip, port, filename, num_threads)){
    case LOCAL:
        { // Braces are required because variables are declared in this block. The braces scope the variables so that
          // other cases do not see them.
#ifndef HAVE_PNGWRITER

            cerr << "This version of the program was not compiled with PNG support and cannot raytrace locally." << endl;
            exit(EXIT_FAILURE);

#else

            ifstream input(filename.c_str());
            if (input.fail()){
                cerr << "Error opening " << filename << endl;
                exit(EXIT_FAILURE);
            }
            string composite_image_name = filename + ".png";

            Raytracer raytracer = processInput(input);
            input.close();

            int resx = raytracer.getX(), resy = raytracer.getY();

            cout << "Raytracing " << resx << "x" << resy << "x" << (raytracer.getAASamples() * raytracer.getAASamples()) << 
                " image with " << ((int) num_threads) << " threads... ";
            cout.flush();
 
            ThreadInfo thread_infos[num_threads];
            for (int i = 0; i < resx; ++i){
                thread_infos[i % num_threads].columns.push_back(i);
            }
            // Divide work by tiles.
            /*
            int tiles_x = ceil(((float) resx) / TILE_SIDE_LENGTH),
                tiles_y = ceil(((float) resy) / TILE_SIDE_LENGTH);
            for (int i = 0; i < tiles_x * tiles_y; ++i){
                thread_infos[i % num_threads].tiles.push_back(i);
            }
            */
        
            // Split up work among the appropriate number of threads.
            boost::thread_group worker_threads;
            for (int i = 0; i < num_threads; ++i){
                boost::thread *thread = new boost::thread(LocalWorkerThread(raytracer, thread_infos[i]));
                worker_threads.add_thread(thread);
            }
    
            worker_threads.join_all();

            cout << "done" << endl;

            /*
            cerr << (resx * resy) << " pixels" << endl;
            cerr << objects_checked << " objects checked: " << (1.0 * objects_checked / (resx * resy)) << " per pixel" << endl;
            cerr << kd_recurses << " kd recurses: " << (1.0 * kd_recurses / (resx * resy)) << " per pixel" << endl;
            */

            cout << "Writing image to file... ";
            cout.flush();
            pngwriter img = pngwriter(resx, resy, 1.0, composite_image_name.c_str());
            img.setcompressionlevel(9);

            // Consolidate tiled pixel data into an image.
            /*
            for (int t = 0; t < num_threads; ++t){
                ThreadInfo ti = thread_infos[t];
                for (unsigned int i = 0; i < ti.tiles.size(); ++i){
                    // C'n'p from localworkerthread.cpp.
                    int x_max = (ti.tiles[i] % tiles_x + 1) * TILE_SIDE_LENGTH,
                        y_max = (ti.tiles[i] / tiles_y + 1) * TILE_SIDE_LENGTH;
                    int pixel_index = 0;
                    for (int x = x_max - TILE_SIDE_LENGTH; x < x_max && x < resx; ++x){
                        for (int y = y_max - TILE_SIDE_LENGTH; y < y_max && y < resy; ++y){
                            Vec3 pixel = ti.pixels[i][pixel_index++];
                            img.plot(x + 1, y + 1, pixel[0], pixel[1], pixel[2]);
                        }
                    }
                }
            }
            */

            // Consolidate all the threads' results into one image and write it.
            for (int t = 0; t < num_threads; ++t){
                for (unsigned int i = 0; i < thread_infos[t].columns.size(); ++i){
                    for (int y = 0; y < resy; ++y){
                        Vec3 pixel = thread_infos[t].pixels[i][y];
                        img.plot(thread_infos[t].columns[i] + 1, y + 1, pixel[0], pixel[1], pixel[2]);
                    }
                }
            }
    
            img.close();

            cout << "done" << endl;
#endif
        }
        break;

    case SERVER:
        {
#ifndef HAVE_PNGWRITER

            cerr << "This version of the program was not compiled with PNG support and cannot be a server." << endl;
            exit(EXIT_FAILURE);

#else

            ifstream input(filename.c_str());
            if (input.fail()){
                cerr << "Error opening " << filename << endl;
                exit(EXIT_FAILURE);
            }
            string composite_image_name = filename + ".png";
        
            cout << "Beginning server on port " << port << "." << endl;
            cout.flush();
            Raytracer raytracer = processInput(input);
        
            input.close();
        
            boost::asio::io_service io;
            Server s(raytracer, composite_image_name, io, port);
            io.run();

#endif
        }
        break;

    case CLIENT:
        {
            // No #ifndef: You can always be a client!
            boost::asio::io_service io;
            Client c(io, host_ip, port, num_threads);
        }
    }
}
