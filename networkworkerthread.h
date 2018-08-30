#ifndef NETWORKWORKERTHREAD_H
#define NETWORKWORKERTHREAD_H

#include <vector>
#include <deque>
#include <ctime>

#include "constants.h"
#include "network.h"
#include "vec3.h"
#include "raytracer.h"

class NetworkWorkerThread{
 public:
    NetworkWorkerThread(const Raytracer &r, string host, string port) : raytracer(r), 
                                                                        twister(time(NULL)),
                                                                        host(host), 
                                                                        port(port),
                                                                        looping(true) {}

    // No destructor! Because this class has to be copyable (required by Boost.Thread),
    // anything that will not already be automatically deallocated (read: pointers) will
    // be COPIED into the new object. The first object gets thrown out immediately, and
    // we don't want its destructor calling delete on the pointer it shares with the
    // valid object.

    void operator()(); // Initiate this client thread.

 private:
    // Perform one column worth of raytracing and store the result.
    void raytraceColumn(int);

    // Begin an asynchronous read from the server for any notices.
    void beginRead();
    
    // Handle the asynchronous read when it comes in, calling one of the following
    // functions and usually calling beginRead() again (if appropriate). Can also
    // be called manually after a blocking read has been performed.
    void handleRead(const boost::system::error_code&);

    // Add the following pixel columns to the queue of columns to do.
    void addColumns();

    // Ask for how many columns to remove, and then call removeColumns(int).
    void removeColumns();

    // Remove the last n columns from the queue of columns to do and report which columns
    // those were to the server.
    void removeColumns(int);

    // Remove any unfinished columns from the queue, send back all the finished columns,
    // and terminate this thread.
    void finish();

    const Raytracer &raytracer;

    // We need independent RNGs for each thread.
    CRandomMersenne twister;

    // Queue of which columns are to be done. Orders to add or remove columns are
    // performed at the end. Columns are popped off the front and raytraced on at a time.
    deque<int> columns_to_do;

    // Finished pixel data.
    vector<PixelColumn> pixels;

    uint8_t out_header, in_header;
    string host, port;
    bool looping;
    tcp::socket *socket;
};

#endif
