#include "inputthread.h"

#include <boost/thread/thread.hpp>
#include <unistd.h>

#include "network.h"

void InputThread::operator()(){
    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query("127.0.0.1", port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    tcp::socket socket(io_service);
    boost::system::error_code error;
    do{
        // unistd.h
        sleep(1);
        socket.close();
        socket.connect(*endpoint_iterator, error);
    } while (error == boost::asio::error::connection_refused); 

    cout << "Waiting for connections." << endl;
    cout << "Type \'start\' and hit enter at any time to beginning raytracing with the currently connected clients." << endl;
    cout << "Type \'quit\' and hit enter to terminate the server." << endl;
    string cmd;
    cin >> cmd;
    while (cmd.compare("start") && cmd.compare("quit")){
        cout << "Recognized commands are \'start\' and \'quit\'." << endl;
        cin >> cmd;
    }
    uint8_t input;
    if (!cmd.compare("start")){
        input = 1;
        // No error checking.
        boost::asio::write(socket, boost::asio::buffer(&input, 1), boost::asio::transfer_all());
    }
    else{
        input = 0;
        boost::asio::write(socket, boost::asio::buffer(&input, 1), boost::asio::transfer_all());
    }
}
