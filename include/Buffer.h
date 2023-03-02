#ifndef _BUFFER_
#define _BUFFER_

#include <Arduino.h>

class Buffer
{
public:
    Buffer();
    Buffer(byte* buf, size_t length);
    Buffer(byte* bufA, size_t lengthA, byte* bufB, size_t lengthB);
    Buffer(Buffer&& buf);
    ~Buffer();

    Buffer  operator+(Buffer& b);
    void    operator=(Buffer&& b);
    Buffer& getMessage();
    byte*   getPtr();
    size_t  getSize();
    void    invalid();

private:
    byte*   fBuffer;
    size_t  fLength;
};

#endif