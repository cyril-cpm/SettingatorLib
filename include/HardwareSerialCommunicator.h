#pragma once

#include "Communicator.h"
#include <queue>

class Message;
class HardwareSerial;

class HardwareSerialCTR: public ICTR
{
    public:

    static HardwareSerialCTR*   CreateInstance(int baudrate = 9600);
    static HardwareSerialCTR*   CreateInstance(HardwareSerial* serial);

    virtual bool        Available() override;
    virtual int         Write(Message& buf) override;
    virtual Message     *Read() override;
    virtual void        Flush();
    virtual void        Update() override;
    virtual uint8_t     GetBoxSize() const override;

    private:
    HardwareSerialCTR(int baudrate);
    HardwareSerialCTR(HardwareSerial* serial);

    void    _receive(Message* msg);
    void    _removeBufferBeginBytes(size_t numberBytes);

    uint8_t*                fSerialBuffer = nullptr;
    size_t                  fSerialBufferSize = 0;
    std::queue<Message*>    fReceivedMessage;
    HardwareSerial*         fSerial;
};