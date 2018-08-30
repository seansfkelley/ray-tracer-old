#ifndef INPUTTHREAD_H
#define INPUTTHREAD_H

#include "constants.h"

class InputThread{
 public:
    InputThread(string port) : port(port) {}

    void operator()();

 private:
    string port;
};

#endif
