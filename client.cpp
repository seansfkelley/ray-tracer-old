#include "client.h"

#include <sstream>
#include <boost/thread/thread.hpp>

#include "networkworkerthread.h"

// This class currently has no error checking. It should have rudimentary checking so that any failures that are not associated with
// a server disconnect (not to be confused with "connection refused", if those errors ever occur. They are likely because the server
// is not quite ready to accept connections, but will be shortly) will result in an attempt to reconnect. It should also attempt to 
// reconnect to the server upon completion.
Client::Client(boost::asio::io_service &io, string hostname, string port, uint8_t num_threads) : socket(io), resolver(io), threads(num_threads),
                                                                                                 host(hostname), port(port){
    cout << "Connecting to " << host << ":" << port << "... ";
    cout.flush();
    connect();
}

void Client::connect(){
    while (true){
        try{
            tcp::resolver::query query(host, port);
            tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
            tcp::resolver::iterator end;
    
            boost::system::error_code error = boost::asio::error::host_not_found;
            while (error && endpoint_iterator != end){
                socket.close();
                socket.connect(*endpoint_iterator++, error);
            }

            // Block until the server successfully reads it. This way, we know the server is ready to communicate with us.
            uint8_t type = HDR_CLIENT;
            boost::asio::write(socket, boost::asio::buffer(&type, 1));
    
            prepare();
        }
        catch (boost::system::system_error se){
            ;
        }
    
        sleep(CLIENT_RETRY_WAIT);
    }
}

void Client::prepare(){
    cout << "connected" << endl;
    // cout << "Identifying self to server... ";
    // cout.flush();
    uint8_t out_header = HDR_CLIENT;
    boost::asio::write(socket, boost::asio::buffer(&out_header, 1));
    // cout << "done" << endl;

    cout << "Downloading raytracer data ";
    cout.flush();
    uint32_t raytracer_size;
    boost::asio::read(socket, boost::asio::buffer(&raytracer_size, 4));
    cout << "(" << formatByteSize(raytracer_size) << ")... ";
    cout.flush();

    char *raytracer_data = new char[raytracer_size];
    boost::asio::read(socket, boost::asio::buffer(raytracer_data, raytracer_size));
    cout << "done" << endl;

    cout << "Closing connection, starting worker threads. " << endl;
    socket.close();

    Raytracer raytracer;
    unarchiveObject(raytracer, raytracer_data, raytracer_size);

    // Spawn threads.
    boost::thread_group worker_threads;
    for (int i = 0; i < threads; ++i){
        boost::thread *thread = new boost::thread(NetworkWorkerThread(raytracer, host, port));
        worker_threads.add_thread(thread);
    }

    worker_threads.join_all();

    cout << endl << "Attemping to reconnect to " << host << ":" << port << "... ";
    cout.flush();
    // Do not call connect(), it will call this function again when appropriate. Let control
    // fall off the end of this function.
}
