#ifndef _SERIAL_COMMUNICATOR_
#define _SERIAL_COMMUNICATOR_

#include "Communicator.h"
#include <HardwareSerial.h>
#include "Buffer.h"
#include "Message.h"

class SerialCTR: public ICTR
{
    public:
    /*
    - create an instance of SerialCTR with baudRate
    */
    static SerialCTR* CreateInstance(int baudRate);

    virtual bool    Available() override;
    virtual int     write(Buffer& buf) override;
    virtual Message Read() override;
    virtual void    Flush(Message& message);
    virtual void    Update() override;

    private:
    SerialCTR(int baudRate);

    HardwareSerial* fSerial = &Serial;
    byte            fBuffer[SERIAL_RX_BUFFER_SIZE];
    uint8_t         fBufferContentSize = 0;
};

#endif