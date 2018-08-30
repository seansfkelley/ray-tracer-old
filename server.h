#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <deque>
#include <map>

#include "constants.h"
#include "network.h"
#include "vec3.h"
#include "raytracer.h"

// A class that allows the server to keep track of what each client thread
// has been assigned to do.
class ClientThreadInfo{
 public:
    ClientThreadInfo() : finishing(false) {}

    uint8_t in_header, out_header;
    uint32_t out_data_size;
    string out_data;
    deque<int> assigned_columns;
    bool finishing;
};

class Server{
 public:
    // Start a server with the given raytracer on the given port.
    Server(Raytracer&, string, boost::asio::io_service&, string);

 private:
    // Begin an asynchronous accept. The server is always looking out for new connections.
    void beginAccept();
    
    // Accept a connection and get it up to speed with the one of the two following
    // functions (as appropriate). Start a new socket listening with beginAccept() unless
    // the error is operation_aborted.
    void acceptConnection(const boost::system::error_code&, tcp::socket*);
    
    // Send the newly connected client the raytracer and other data so it can spawn some
    // worker threads.
    void prepareClient(tcp::socket*);

    // Get the newly connected client thread ready to go and start listening for updates
    // from it.
    void prepareClientThread(tcp::socket*);

    // Ensure the write didn't produce and error. Perform no follow-up action.
    void handleWriteDefault(const boost::system::error_code&, tcp::socket*);

    // Start an asynchronous read for the given socket (a client thread).
    void beginClientThreadRead(tcp::socket*);

    // Perform the appropriate action when a client sends an update to this server. (Add/
    // remove columns, tell client thread to finish, etc...).
    void handleClientThreadRead(const boost::system::error_code&, tcp::socket*);

    // Start a blocking write of whatever is in the appropriate ClientThreadInfo to the
    // given client thread.
    void beginClientThreadWrite(tcp::socket*);

    // Handle certain communication errors (usually by disconnecting from the problematic
    // client) and return whether or not there was an error at all.
    bool checkError(const boost::system::error_code&, tcp::socket*);
    
    // Cancel all operations on the socket currently waiting on new connections, causing
    // an operation_aborted error. beginAccept() must be called manually if connections
    // are to be listened for again.
    void stopAccept();

    // Assemble the information in finished_columns into an image and write it to disk.
    void writeImage();

    // Close all client connections cleanly and shut down this process. Does not write
    // any image to disk, regardless of state.
    void shutdownServer();

    // Pre-archived raytracer information.
    string image_name, raytracer_data;
    uint32_t raytracer_data_length;

    // The resolution of the output image.
    unsigned int resx, resy;
    
    boost::asio::io_service &io;
    tcp::acceptor acceptor;

    // The socket that is currently listening for new connections. Upon connection, it
    // will be handled appropriately and a new socket will be put in its place.
    tcp::socket *new_connection_socket;

    // Each client thread is uniquely identified by its socket. This maps sockets onto
    // ClientThreadInfos to allow storage of more information specific to each client
    // thread.
    map<tcp::socket*, ClientThreadInfo> client_threads;

    // Store all columns that client threads have completed and sent back.
    vector<PixelColumn> finished_columns;

    // Track which columns are not currently assigned to any client.
    deque<int> columns_to_do;
};

#endif
