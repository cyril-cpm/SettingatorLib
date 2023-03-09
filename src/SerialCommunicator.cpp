#include "SerialCommunicator.h"

SerialCTR::SerialCTR(int baudRate)
{
    fSerial->begin(baudRate);
}

SerialCTR* SerialCTR::CreateInstance(int baudRate)
{
    return new SerialCTR(baudRate);
}

bool SerialCTR::available()
{
    if (fSerial->available())
        update();

    return fBuffer.getSize() > 0;
}

Message SerialCTR::read()
{
    if (available())
        return fBuffer.getMessage();
    return Message();
}

int SerialCTR::write(Buffer& buf)
{
    return fSerial->write(buf.getPtr(), buf.getSize());
}

void SerialCTR::update()
{
    int availableBytes = fSerial->available();
    byte* buffer = (byte*)malloc(availableBytes * sizeof(byte));
    int readedBytes = fSerial->readBytes(buffer, availableBytes);

    if (readedBytes > 0)
        fBuffer = fBuffer + Buffer(buffer, readedBytes);
}