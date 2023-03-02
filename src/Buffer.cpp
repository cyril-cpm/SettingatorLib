#include "Buffer.h"

Buffer::Buffer()
{
    fBuffer = nullptr;
    fLength = 0;
}

Buffer::Buffer(byte* buf, size_t length)
{
    fBuffer = (byte*)malloc(length * sizeof(byte));
    memmove(fBuffer, buf, length);
    fLength = length;
}

Buffer::Buffer(byte* bufA, size_t lengthA, byte* bufB, size_t lengthB)
{
    fBuffer = (byte*)malloc((lengthA + lengthB) * sizeof(byte));
    memmove(fBuffer, bufA, lengthA);
    memmove(fBuffer + lengthA, bufB, lengthB);
    fLength = lengthA + lengthB;
}

Buffer::Buffer(Buffer&& buf)
{
    fBuffer = buf.getPtr();
    fLength = buf.getSize();

    buf.invalid();
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

void Buffer::operator=(Buffer&& b)
{
    fBuffer = b.getPtr();
    fLength = b.getSize();

    b.invalid();
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