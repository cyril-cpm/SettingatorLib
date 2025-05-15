#pragma once

#if defined(ARDUINO)

#include "Communicator.h"

class Message;
class HardwareSerial;

class HardwareSerialCTR: public ICTR
{
    public:

    static HardwareSerialCTR*   CreateInstance(int baudrate = 9600);
    static HardwareSerialCTR*   CreateInstance(HardwareSerial* serial);

    virtual int         Write(Message& buf) override;
    virtual void        Update() override;

    private:
    HardwareSerialCTR(int baudrate);
    HardwareSerialCTR(HardwareSerial* serial);

    void    _removeBufferBeginBytes(size_t numberBytes);

    uint8_t*                fSerialBuffer = nullptr;
    size_t                  fSerialBufferSize = 0;
    HardwareSerial*         fSerial;
};

#endif