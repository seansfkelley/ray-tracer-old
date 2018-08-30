#ifndef ZLIBSTRING_H
#define ZLIBSTRING_H

#include <zlib.h>

#include "constants.h"

// (Un)compress a string of raw data using zlib.
string compress_string(const string &str, int compressionlevel = Z_BEST_COMPRESSION);
string decompress_string(const string &str);

#endif
