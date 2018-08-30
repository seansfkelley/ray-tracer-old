#include "networkworkerthread.h"

void NetworkWorkerThread::operator()(){
    boost::asio::io_service io;
    socket = new tcp::socket(io);
    tcp::resolver resolver(io);
    tcp::resolver::query query(host, port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;
    
    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end){
        socket->close();
        socket->connect(*endpoint_iterator++, error);
    }

    if (error){
        throw boost::system::system_error(error);
    }

    cout << "Client thread connected." << endl;

    out_header = HDR_CLIENT_THREAD;
    boost::asio::write(*socket, boost::asio::buffer(&out_header, 1));

    beginRead();

    try{
        while (looping){
            io.poll();
            // It is not necessary to have mutexes to lock access to columns_to_do or any other
            // variables: "The io_service guarantees that the handler will only be called in a 
            // thread in which the run(), run_one(), poll() or poll_one() member functions is 
            // currently being invoked." (From Asio documentation).
            if (!columns_to_do.empty()){
                int col = columns_to_do.front();
                columns_to_do.pop_front();
                raytraceColumn(col);
            } 
            else{
                out_header = HDR_WANT_MORE;
                socket->cancel();
                boost::asio::write(*socket, boost::asio::buffer(&out_header, 1));
                boost::asio::read(*socket, boost::asio::buffer(&in_header, 1));
                handleRead(boost::system::errc::make_error_code(boost::system::errc::success));
            }
        }
    }
    catch (boost::system::system_error se){
        cerr << "Connection unexpectedly terminated, closing connection." << endl;
    }

    try{
        socket->cancel();
        socket->shutdown(tcp::socket::shutdown_both);
        socket->close();
    }
    catch (boost::system::system_error se){
        ;
    }

    // This class has no destructor, the memory allocated on the heap is freed
    // here. Read the header for the justification. 
    delete socket;
}

void NetworkWorkerThread::raytraceColumn(int col){
    int resy = raytracer.getY();

    PixelColumn pc;
    pc.column = col;
    pc.pixels.reserve(resy);
    
    for (int y = 0; y < resy; ++y){
        Vec3 p_vec = raytracer.colorTrace(col, y, twister) * 255;
        Pixel p;
        p.r = (uint8_t) p_vec.x;
        p.g = (uint8_t) p_vec.y;
        p.b = (uint8_t) p_vec.z;
        pc.pixels.push_back(p);
    }

    pixels.push_back(pc);
}

void NetworkWorkerThread::beginRead(){
    boost::asio::async_read(*socket, 
                            boost::asio::buffer(&in_header, 1), 
                            boost::bind(&NetworkWorkerThread::handleRead,
                                        this,
                                        boost::asio::placeholders::error));
}

void NetworkWorkerThread::handleRead(const boost::system::error_code &error){
    if (error == boost::asio::error::operation_aborted){
        return;
        // Ignore: this is because the async_read was cancelled in favor of the
        // blocking write/read pair when the thread is requesting more work.
    }
    else if (error){
        throw boost::system::system_error(error);
    }

    switch (in_header){
    case HDR_ADD_COL:
        addColumns();
        beginRead();
        break;

    case HDR_REM_COL:
        removeColumns();
        beginRead();
        break;

    case HDR_FINISH:
        finish();
        looping = false;
        break;

    default:
        cerr << "Unknown header \'" << ((int) in_header) << "\', ignoring." << endl;
        beginRead();
    }
}

void NetworkWorkerThread::addColumns(){
    uint32_t data_size;
    boost::asio::read(*socket, boost::asio::buffer(&data_size, 4));
    char *data = new char[data_size];
    boost::asio::read(*socket, boost::asio::buffer(data, data_size));
    vector<int> add_cols; 
    unarchiveObject(add_cols, data, data_size);
    columns_to_do.insert(columns_to_do.end(), add_cols.begin(), add_cols.end());
}

void NetworkWorkerThread::removeColumns(){
    uint32_t how_many;
    boost::asio::read(*socket, boost::asio::buffer(&how_many, 4));
    
    removeColumns(how_many);
}

void NetworkWorkerThread::removeColumns(int how_many){
    uint32_t num_removed = min<int>(how_many, columns_to_do.size());
    columns_to_do.resize(columns_to_do.size() - num_removed);

    out_header = HDR_REM_COL;
    boost::asio::write(*socket, boost::asio::buffer(&out_header, 1));
    
    boost::asio::write(*socket, boost::asio::buffer(&num_removed, 4));
}

void NetworkWorkerThread::finish(){
    if (columns_to_do.size() > 0){
        removeColumns(columns_to_do.size());
    }

    cout << "Compressing finished image data... ";
    cout.flush();
    string data = archiveObject(pixels);
    uint32_t data_size = data.length();
    cout << "compressed" << endl;

    cout << "Sending " << formatByteSize(data_size) << " of image data to server... ";
    cout.flush();

    out_header = HDR_FINISH;
    boost::asio::write(*socket, boost::asio::buffer(&out_header, 1));

    boost::asio::write(*socket, boost::asio::buffer(&data_size, 4));
    boost::asio::write(*socket, boost::asio::buffer(data));

    cout << "sent" << endl;
}
