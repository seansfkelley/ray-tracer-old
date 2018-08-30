#ifndef LOCALWORKERTHREAD_H
#define LOCALWORKERTHREAD_H

#include <vector>

#include "constants.h"
#include "vec3.h"
#include "raytracer.h"

struct ThreadInfo{
    // Which columns this thread is to compute.
    vector<int> columns;
    // Use this when doing the tiling version...
    // vector<int> tiles;
    
    // Resultant pixel tiles. The contents of this aren't guaranteed 
    // to be complete until the thread terminates.
    vector<vector<Vec3> > pixels; 
};

class LocalWorkerThread{
 public:
    LocalWorkerThread(const Raytracer&, ThreadInfo&);

    // Do all assigned work. Modifying the contents of the threadinfo
    // class while this thread is running is undefined behavior.
    void operator()(); 

 private:
    const Raytracer &raytracer;
    ThreadInfo &info;
};

#endif
