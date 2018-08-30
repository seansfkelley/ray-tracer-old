#ifndef NETWORK_H
#define NETWORK_H

#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <stdint.h>

#include "constants.h"
#include "zlibstring.h"
#include "vec3.h"

using boost::asio::ip::tcp;

// How many seconds to wait between connection attempts for the client.
const int CLIENT_RETRY_WAIT = 3;

const string DEFAULT_PORT = "3678";

// List of the recognized types of headers.
enum {
      HDR_CLIENT = 0,    // Client is connecting.
      HDR_CLIENT_THREAD, // Client thread is connecting.
      HDR_WANT_MORE,     // Client thread wants more work to do.
      HDR_ADD_COL,       // Client thread should add the following columns.
      HDR_REM_COL,       // Client thread should remove the following columns.
      HDR_FINISH         // Client thread should stop and send back all data.
};

class Pixel{
 public:
    uint8_t r, g, b;

 private:
    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & r;
        ar & g;
        ar & b;
    }
};

class PixelColumn{
 public:
    // Which column, starting with 0 on the left of the image, this is.
    int column;

    // Column pixel data.
    vector<Pixel> pixels; 

 private:
    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & column;
        ar & pixels;
    }
};

// Formats the given number of bytes into a smaller number with the appropriate suffix (B, KB, MB).
inline string formatByteSize(unsigned int bytes){
    float bytesf = bytes;
    stringstream ss;
    if (bytesf > 4096){
        ss.precision(1);
        bytesf /= 1024;
        if (bytesf > 1024){
            ss << fixed << (bytesf / 1024) << "MB";
        }
        else{
            ss << fixed << bytesf << "KB";
        }
    }
    else{
        ss << bytes << "B";
    }
    return ss.str();
}

// Archives and then compresses the given object. This function should always be used for all
// outgoing serialized objects.
template<class T>
inline string archiveObject(T &obj){
    string data;
    stringstream ss;
    boost::archive::text_oarchive out_archive(ss);
    out_archive << obj;
    data = ss.str();
    return compress_string(data);
    // return data;
}

// Uncompresses and then unarchives the given bytes. This function should always be used for
// all incoming serialized objects.
template<class T>
inline void unarchiveObject(T &obj, char *data, uint32_t data_size){
    // The string must be forced to be the right size. Otherwise, non-null characters
    // at the end of 'data' will be included in the stringstream, which, if they are
    // of the right type, can change the resultant data structure.
    stringstream ss(decompress_string(string(data, data_size)));
    // stringstream ss(string(data, data_size));
    boost::archive::text_iarchive in_archive(ss);
    in_archive >> obj;
    delete [] data;
}

#endif
