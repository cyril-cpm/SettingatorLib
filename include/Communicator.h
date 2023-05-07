#ifndef _COMMUNICATOR_
#define _COMMUNICATOR_

#include <Arduino.h>
#include "Buffer.h"
#include "Message.h"

class ICTR
{
    public:

    /*
    - return true if there is bytes available to read
    */
    virtual bool available() = 0;

    /*
    - Write Buffer to communicator
    */
    virtual int write(Buffer& buf) = 0;

    /*
    - Read a message if avaible or return empty Message
     */
    virtual Message read() = 0;

    /*
    - Update internal Buffer
    */
    virtual void update() = 0;
};

#endif