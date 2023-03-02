#ifndef _SERIAL_COMMUNICATOR_
#define _SERIAL_COMMUNICATOR_

#include "Communicator.h"
#include <HardwareSerial.h>
#include "Buffer.h"

class SerialCTR: public ICTR
{
    public:
    static SerialCTR* CreateInstance(int baudRate);

    virtual bool available() override;
    virtual int write(Buffer& buf) override;
    virtual Buffer read() override;
    virtual void update() override;

    private:
    SerialCTR(int baudRate);

    HardwareSerial* fSerial = &Serial;
    Buffer          fBuffer;
};

#endif