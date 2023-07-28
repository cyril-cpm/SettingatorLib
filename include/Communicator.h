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
    virtual bool Available() = 0;

    /*
    - Write Buffer to communicator
    */
    virtual int Write(Message& buf) = 0;

    /*
    - Read a message if avaible or return empty Message
     */
    virtual Message* Read() = 0;

    /*
    - Flush message after having executed
    */
    virtual void    Flush(Message& message) = 0;

    /*
    - Update internal Buffer
    */
    virtual void Update() = 0;
};

#endif