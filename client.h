#ifndef CLIENT_H
#define CLIENT_H

#include "constants.h"
#include "network.h"

class Client{
 public:
    Client(boost::asio::io_service&, string, string, uint8_t);

 private:
    // Attempt to initiate a connect with the server. On failure, wait a few seconds
    // and try again. Clients run in the background of the user's computer, occasionally 
    // querying the server they were started with to ask if there's any more work. This
    // keeps the work the user has to do to contribute a client to the ray tracer to a 
    // bare minimum.
    void connect();

    // Download raytracer data and spawn the appropriate number of worker threads to
    // perform the work. Each thread handles its own communication with the server. Upon
    // completion, the main client process (this) will revert back to occasional
    // reconnect attempts.
    void prepare();


    tcp::socket socket;
    tcp::resolver resolver;
    uint8_t threads;
    string host, port;
};

#endif
