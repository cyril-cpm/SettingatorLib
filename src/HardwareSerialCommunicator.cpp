#if defined(ARDUINO)

#include "HardwareSerialCommunicator.h"

#include "MiscDef.h"
#include "Message.h"

#include <HardwareSerial.h>

HardwareSerialCTR::HardwareSerialCTR(int baudrate)
{
    fSerial = &Serial;

    /*fSerial->setRxTimeout(1);
    fSerial->onReceive([this](){
        size_t messageLen = fSerial->available();
        uint8_t* buffer = (uint8_t*)malloc(messageLen * sizeof(uint8_t));
        fSerial->readBytes(buffer, messageLen);
        _receive(Message(buffer, messageLen));
        free(buffer);
    }, true);*/

    fSerial->begin(baudrate);
}

HardwareSerialCTR* HardwareSerialCTR::CreateInstance(int baudrate)
{
    return new HardwareSerialCTR(baudrate);
}


int HardwareSerialCTR::Write(Message& msg)
{
    fSerial->write(msg.GetBufPtr(), msg.GetLength());
    return 0;
}

void HardwareSerialCTR::Update()
{
    int n = 0;
    if ((n = fSerial->available()))
    {
        
       // Serial.print("LOG Available on Serial\tn: ");
       // Serial.println(n);

        uint8_t* newBuffer = (uint8_t*)malloc(fSerialBufferSize + n);

        if (fSerialBufferSize)
        {
          //  Serial.println("Freeing Serial Buffer");
            memcpy(newBuffer, fSerialBuffer, fSerialBufferSize);
            free(fSerialBuffer);
        }

        fSerial->read(newBuffer + fSerialBufferSize, n);

        fSerialBuffer = newBuffer;
        fSerialBufferSize += n;
    }

    //Serial.write(fSerialBuffer, fSerialBufferSize);

    int i = 0;
    for (i = 0; i < fSerialBufferSize && fSerialBuffer[i] != Message::Frame::Start; i++);

    if (i != 0)
        _removeBufferBeginBytes(i);
    
    if (fSerialBufferSize >= 5)
    {
        size_t msgSize = (fSerialBuffer[1] << 8) + fSerialBuffer[2];

        if (fSerialBufferSize >= msgSize)
        {
            if (fSerialBuffer[msgSize - 1] == Message::Frame::End)
            {
                //Serial.println("LOG Valid message");
                _receive(Message(fSerialBuffer, msgSize));
                _removeBufferBeginBytes(msgSize);

            }
            //else
                //Serial.println("LOG invalid message");
        }
    }

    return;
}

void HardwareSerialCTR::_removeBufferBeginBytes(size_t numberBytes)
{
   // Serial.print("LOG _removeBeginBytes numberBytes:");
   // Serial.print(numberBytes);
   // Serial.print("\tfSerialBufferSize: ");
   // Serial.println(fSerialBufferSize);
    
    uint8_t* newBuffer = (uint8_t*)malloc(fSerialBufferSize - numberBytes);

    memcpy(newBuffer, fSerialBuffer + numberBytes, fSerialBufferSize - numberBytes);
    free(fSerialBuffer);
    fSerialBuffer = newBuffer;

    fSerialBufferSize = fSerialBufferSize - numberBytes;
}

#endif