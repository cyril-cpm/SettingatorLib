#include "Buffer.h"
#include "Message.h"

uint64_t getIntegerFromBuffer(size_t numberOfBytes, byte* buffer)
{
    uint64_t value = 0;

    for (size_t i = 0; i < numberOfBytes; ++i)
    {
        value <<= value;
        value += buffer[i];
    }

    return value;
}

Buffer::Buffer()
{
    fBuffer = nullptr;
    fLength = 0;
}

Buffer::Buffer(byte* buf, size_t length)
{
    fBuffer = (byte*)malloc(length * sizeof(byte));
    memcpy(fBuffer, buf, length);
    fLength = length;
}

Buffer::Buffer(byte* bufA, size_t lengthA, byte* bufB, size_t lengthB)
{
    fBuffer = (byte*)malloc((lengthA + lengthB) * sizeof(byte));
    memcpy(fBuffer, bufA, lengthA);
    memcpy(fBuffer + lengthA, bufB, lengthB);
    fLength = lengthA + lengthB;
}

#define STEAL_BUFFER(buf)   {                           \
                                fBuffer=buf.getPtr();   \
                                fLength=buf.getSize();  \
                                buf.invalid();          \
                            }

#define COPY_BUFFER(buf)    {                                                           \
                                fBuffer=(byte*)memcpy(fBuffer, buf.getPtr(), buf.getSize());   \
                                fLength=buf.getSize();                                  \
                            }

Buffer::Buffer(Buffer&& buf)
{
    STEAL_BUFFER(buf)
}

Buffer::Buffer(Buffer& buf)
{
    COPY_BUFFER(buf)
}

Buffer::~Buffer()
{
    if (fBuffer != nullptr)
        free(fBuffer);
}

Buffer Buffer::operator+(Buffer& B)
{
    return Buffer(fBuffer, fLength, B.getPtr(), B.getSize());
}

Buffer Buffer::operator+(Buffer&& B)
{
    return Buffer(fBuffer, fLength, B.getPtr(), B.getSize());
}

void Buffer::operator=(Buffer&& buf)
{
    STEAL_BUFFER(buf)
}

void Buffer::operator=(Buffer& buf)
{
    COPY_BUFFER(buf)
}

byte Buffer::operator[](uint16_t index)
{
    if (index < fLength)
        return -1;
    return fBuffer[index];
}

Message Buffer::getMessage()
{
    size_t messageLen;

    bool valid = true;

    if (valid)
        valid = (fBuffer[0] == Message::Frame::Start);

    if (valid)
        messageLen = GET_16BIT_INT(fBuffer + 1);

    if (valid)
        valid = (fBuffer[messageLen - 1] == Message::Frame::End);

    Buffer remainantBuffer(fBuffer + messageLen, fLength - messageLen + 1);

    Message message = Message(Buffer(fBuffer, messageLen));

    *this = remainantBuffer;

    return message;
}

byte* Buffer::getPtr()
{
    return fBuffer;
}

size_t Buffer::getSize()
{
    return fLength;
}

void Buffer::invalid()
{
    fBuffer = nullptr;
}