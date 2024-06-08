#ifndef _COMMUNICATOR_
#define _COMMUNICATOR_

#include <sys/_stdint.h>
#include <queue>

class Message;

class ICTR
{
    public:

    /*
    - return true if there is bytes available to read
    */
    virtual bool Available();

    /*
    - Write Buffer to communicator
    */
    virtual int Write(Message& buf) = 0;

    /*
    - Read a message if avaible or return empty Message
     */
    virtual Message* Read();

    /*
    - Flush message after having executed
    */
    virtual void    Flush();

    /*
    - Update internal Buffer
    */
    virtual void Update() = 0;

    virtual uint8_t GetBoxSize() const;

    protected:

    void _receive(Message* msg);

    std::queue<Message*> fReceivedMessage;

};

#endif