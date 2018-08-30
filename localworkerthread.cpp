#include "localworkerthread.h"

LocalWorkerThread::LocalWorkerThread(const Raytracer &r, ThreadInfo &ti) : raytracer(r), info(ti){
    for (unsigned int i = 0; i < info.columns.size(); ++i){
        info.pixels.push_back(vector<Vec3>());
    }
}

void LocalWorkerThread::operator()(){
    CRandomMersenne twister(time(NULL));

    int resy = raytracer.getY();
    for (unsigned int i = 0; i < info.columns.size(); ++i){
        for (int y = 0; y < resy; ++y){
            info.pixels[i].push_back(raytracer.colorTrace(info.columns[i], y, twister));
        }
    }
}

// Tiling version. Maybe this will be useful again sometime...?
/*
LocalWorkerThread::LocalWorkerThread(const Raytracer &r, ThreadInfo &ti) : raytracer(r), info(ti){
    for (unsigned int i = 0; i < info.tiles.size(); ++i){
        info.pixels.push_back(vector<Vec3>());
    }
}

void LocalWorkerThread::operator()(){
    srand(time(NULL));

    int resx = raytracer.getX(), resy = raytracer.getY();
    int tiles_x = ceil(((float) resx) / TILE_SIDE_LENGTH),
        tiles_y = ceil(((float) resy) / TILE_SIDE_LENGTH);

    for (unsigned int i = 0; i < info.tiles.size(); ++i){
        int x_max = (info.tiles[i] % tiles_x + 1) * TILE_SIDE_LENGTH,
            y_max = (info.tiles[i] / tiles_y + 1) * TILE_SIDE_LENGTH; // This works because of integer division.
        for (int x = x_max - TILE_SIDE_LENGTH; x < x_max && x < resx; ++x){
            for (int y = y_max - TILE_SIDE_LENGTH; y < y_max && y < resy; ++y){
                info.pixels[i].push_back(raytracer.colorTrace(x, y));
            }
        }
    }
}
*/
