#ifndef _BUFFER_
#define _BUFFER_

#include <Arduino.h>

class Message;

uint64_t getIntegerFromBuffer(size_t numberOfBytes, byte* buffer);

#define GET_16BIT_INT(X) getIntegerFromBuffer(2, X)

class Buffer
{
public:
    Buffer();
    Buffer(byte* buf, size_t length);
    Buffer(byte* bufA, size_t lengthA, byte* bufB, size_t lengthB);
    Buffer(Buffer&& buf);
    Buffer(Buffer& buf);
    ~Buffer();

    Buffer  operator+(Buffer& b);
    Buffer  operator+(Buffer&& b);
    void    operator=(Buffer&& b);
    void    operator=(Buffer& buf);
    byte    operator[](uint16_t index);

    Message getMessage();
    byte*   getPtr();
    size_t  getSize();
    void    invalid();

private:
    byte*   fBuffer;
    size_t  fLength;
};

#endif