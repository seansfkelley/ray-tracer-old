#include "server.h"

#include <vector>

#ifdef HAVE_PNGWRITER
#include <pngwriter.h>
#endif

#define CHECKERROR(e, s) if (checkError(e, s)) return;

Server::Server(Raytracer &rt, string filename, boost::asio::io_service &io, string port) : image_name(filename), resx(rt.getX()), resy(rt.getY()), 
                                                                                           io(io), acceptor(io, tcp::endpoint(tcp::v4(), atoi(port.c_str()))){
    cout << "Compressing raytracer data... ";
    cout.flush();
    raytracer_data = archiveObject(rt);
    raytracer_data_length = raytracer_data.length();
    cout << formatByteSize(raytracer_data_length) << endl;
    cout << "Waiting for incoming connections to automatically start." << endl;

    for (unsigned int i = 0; i < resx; ++i){
        columns_to_do.push_back(i);
    }

    beginAccept();
}

void Server::beginAccept(){
    new_connection_socket = new tcp::socket(io);
    acceptor.async_accept(*new_connection_socket, 
                          boost::bind(&Server::acceptConnection, 
                                      this, 
                                      boost::asio::placeholders::error,
                                      new_connection_socket));
}

void Server::acceptConnection(const boost::system::error_code &error, tcp::socket *socket){
    // We purposely cancelled the async_connect, so don't even bother checking anything.
    if (error== boost::asio::error::operation_aborted){
        return;
    }

    beginAccept();

    if (!error){
        boost::system::error_code err;
        uint8_t client_type;
        boost::asio::read(*socket, boost::asio::buffer(&client_type, 1), boost::asio::transfer_all(), err);
        CHECKERROR(error, socket);

        if (client_type == HDR_CLIENT){
            prepareClient(socket);
        }
        else if (client_type == HDR_CLIENT_THREAD){
            prepareClientThread(socket);
        }
        else{
            cerr << "Unrecognized client type \'" << client_type << "\', refusing connection." << endl;
            socket->close();
            delete socket;
        }
    }
    else{
        cerr << "There was an error while connecting to a new client. Closing connection." << endl;
        socket->close();
        delete socket;
    }
}

void Server::prepareClient(tcp::socket *socket){
    boost::system::error_code error;
    cout << "Connected to new client, sending raytracer data... ";
    cout.flush();
    boost::asio::write(*socket, boost::asio::buffer(&raytracer_data_length, 4), boost::asio::transfer_all(), error);
    CHECKERROR(error, socket);
    cout << "done" << endl;

    boost::asio::async_write(*socket, 
                             boost::asio::buffer(raytracer_data),
                             boost::bind(&Server::handleWriteDefault, 
                                         this,
                                         boost::asio::placeholders::error,
                                         socket));
}

void Server::prepareClientThread(tcp::socket *socket){
    cout << "Connected to new client thread (" << (client_threads.size() + 1) << " clients connected)." << endl;

    beginClientThreadRead(socket);
}

void Server::handleWriteDefault(const boost::system::error_code &error, tcp::socket *socket){
    CHECKERROR(error, socket);
}

void Server::beginClientThreadRead(tcp::socket *socket){
    // This will create a new instance if this a new connection.
    ClientThreadInfo &cti = client_threads[socket];

    boost::asio::async_read(*socket, 
                            boost::asio::buffer(&cti.in_header, 1),
                            boost::bind(&Server::handleClientThreadRead, 
                                        this,
                                        boost::asio::placeholders::error,
                                        socket));
}

void Server::handleClientThreadRead(const boost::system::error_code &error, tcp::socket *socket){
    CHECKERROR(error, socket);

    boost::system::error_code err;
    ClientThreadInfo &cti = client_threads[socket];

    switch(cti.in_header){
    case HDR_WANT_MORE:
        if (columns_to_do.empty()){
            cti.finishing = true;
            cti.out_header = HDR_FINISH;
            cti.out_data_size = 0;
        }
        else{
            int col = columns_to_do.front();
            columns_to_do.pop_front();
            cti.out_header = HDR_ADD_COL;
            cti.assigned_columns.push_back(col);
            vector<int> add_cols;
            add_cols.push_back(col);
            cti.out_data = archiveObject(add_cols);
            cti.out_data_size = cti.out_data.length();
        }
        beginClientThreadWrite(socket);
        beginClientThreadRead(socket);
        break;

    case HDR_REM_COL:
        uint32_t num_removed;
        boost::asio::read(*socket, boost::asio::buffer(&num_removed, 4), boost::asio::transfer_all(), err);
        CHECKERROR(err, socket);

        for (; num_removed > 0; --num_removed){
            columns_to_do.push_back(cti.assigned_columns.back());
            cti.assigned_columns.pop_back();
        }

        beginClientThreadRead(socket);
        break;

    case HDR_FINISH:
        // Brackets make the compiler STFU about crossing initializations.
        {
            uint32_t data_size;
            boost::asio::read(*socket, boost::asio::buffer(&data_size, 4), boost::asio::transfer_all(), err);
            CHECKERROR(err, socket);
            char *data = new char[data_size];
            boost::asio::read(*socket, boost::asio::buffer(data, data_size), boost::asio::transfer_all(), err);
            CHECKERROR(err, socket);
            
            // Wait for the client to close the connection cleanly.
            try{
                // Do not pass in err to read, we want it thrown.
                uint8_t dummy;
                boost::asio::read(*socket, boost::asio::buffer(&dummy, 1));
            }
            catch (boost::system::system_error se){
                if (se.code() == boost::asio::error::eof){
                    cout << "Client thread disconnected after finishing, incorporating data... ";
                    cout.flush();
                    // socket->shutdown() is handled by the client.
                    socket->close();
                    client_threads.erase(socket);
                    delete socket;
                }
                else{
                    CHECKERROR(se.code(), socket);
                }
            }
            
            vector<PixelColumn> new_columns;
            unarchiveObject(new_columns, data, data_size);
            finished_columns.insert(finished_columns.end(), new_columns.begin(), new_columns.end());

            cout << "done (" << client_threads.size() << " clients connected)." << endl;

            if (finished_columns.size() == resx){
                stopAccept();
                writeImage();
                shutdownServer();
            }
        }
        // No more reads on this socket, of course.
        break;

    default:
        cerr << "Unrecognized client request \'" << ((int) cti.in_header) << "\', ignoring." << endl;
        beginClientThreadRead(socket);
    }
}

void Server::beginClientThreadWrite(tcp::socket *socket){
    boost::system::error_code error;
    ClientThreadInfo &cti = client_threads[socket];

    boost::asio::write(*socket, boost::asio::buffer(&cti.out_header, 1), boost::asio::transfer_all(), error);
    CHECKERROR(error, socket);
    if (cti.out_data_size > 0){
        boost::asio::write(*socket, boost::asio::buffer(&cti.out_data_size, 4), boost::asio::transfer_all(), error);
        CHECKERROR(error, socket);
        boost::asio::async_write(*socket,
                                 boost::asio::buffer(cti.out_data),
                                 boost::bind(&Server::handleWriteDefault,
                                             this,
                                             boost::asio::placeholders::error,
                                             socket));
    }
}

bool Server::checkError(const boost::system::error_code &error, tcp::socket *socket){
    if (error){
        if (error == boost::asio::error::operation_aborted){
            // Return true: we purposely aborted the connection, so we don't want whatever
            // it produces to be processed. But it's also not really an error, so don't
            // print any disagnostic information.
            return true;
        }
        map<tcp::socket*, ClientThreadInfo>::iterator entry = client_threads.find(socket);
        if (entry == client_threads.end()){
            cerr << "There was an error while communicating with a client: " << error << ". Closing connection." << endl;
            socket->close();
            delete socket;
        }
        else{
            cerr << "There was an error while communicating with a client thread: " << error << ". Closing connection." << endl;
            ClientThreadInfo &cti = client_threads[socket];
            columns_to_do.insert(columns_to_do.end(), cti.assigned_columns.begin(), cti.assigned_columns.end());
            socket->close();
            client_threads.erase(entry);
            delete socket;
        }
        return true;
    }
    return false;
}

void Server::stopAccept(){
    // new_connection_socket->cancel();
    // delete new_connection_socket;
}

void Server::writeImage(){
#ifdef HAVE_PNGWRITER
    cout << "Writing image to disk... ";
    cout.flush();
    
    pngwriter image(resx, resy, 1.0, image_name.c_str());
    image.setcompressionlevel(9);

    vector<PixelColumn>::iterator c_iter;
    for (c_iter = finished_columns.begin(); c_iter != finished_columns.end(); ++c_iter){
        int x = (*c_iter).column;
        for (unsigned int y = 0; y < resy; ++y){
            Pixel p = (*c_iter).pixels[y];
            image.plot(x + 1, y + 1, ((int) p.r) << 8, ((int) p.g) << 8,((int) p.b) << 8);
        }
    }

    image.close();

    cout << "done" << endl;
#endif
}

void Server::shutdownServer(){
    cout << "Shutting down server... ";
    cout.flush();

    map<tcp::socket*, ClientThreadInfo>::iterator cti_iter;
    for (cti_iter = client_threads.begin(); cti_iter != client_threads.end(); ++cti_iter){
        tcp::socket *socket = (*cti_iter).first;
        socket->cancel();
        socket->shutdown(tcp::socket::shutdown_both);
        socket->close();
        delete socket;
    }
    client_threads.clear();

    cout << "done" << endl;

    io.stop();
}
