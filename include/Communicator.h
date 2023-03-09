#ifndef _COMMUNICATOR_
#define _COMMUNICATOR_

#include <Arduino.h>
#include "Buffer.h"
#include "Message.h"

class ICTR
{
    public:
    virtual bool available() = 0;
    virtual int write(Buffer& buf) = 0;
    virtual Message read() = 0;
    virtual void update() = 0;
};

#endif