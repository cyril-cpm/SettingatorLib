#include "SerialCommunicator.h"

SerialCTR::SerialCTR(int baudRate)
{
    fSerial->begin(baudRate);
}

SerialCTR* SerialCTR::CreateInstance(int baudRate)
{
    return new SerialCTR(baudRate);
}

bool SerialCTR::Available()
{
    if (fSerial->available())
        Update();

    return fBufferContentSize > 0;
}

Message SerialCTR::Read()
{
    if (Available())
        return Message(fBuffer);
    return Message();
}

void SerialCTR::Flush(Message& message)
{
    uint8_t messageLength = message.GetLength();
    uint8_t remainingByteInBuf = fBufferContentSize - messageLength;

    memmove(fBuffer, fBuffer + messageLength, remainingByteInBuf);
    memset(fBuffer + remainingByteInBuf, 0, SERIAL_RX_BUFFER_SIZE - remainingByteInBuf);

    fBufferContentSize = remainingByteInBuf;
}

int SerialCTR::write(Buffer& buf)
{
    return fSerial->write(buf.getPtr(), buf.getSize());
}

void SerialCTR::Update()
{
    int readedBytes = fSerial->readBytes(fBuffer + fBufferContentSize, SERIAL_RX_BUFFER_SIZE);
    fBufferContentSize += readedBytes;
}