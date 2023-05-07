#ifndef _BUFFER_
#define _BUFFER_

#include <Arduino.h>

class Message;

uint64_t getIntegerFromBuffer(size_t numberOfBytes, byte* buffer);

#define GET_16BIT_INT(X) getIntegerFromBuffer(2, X)

class Buffer
{
public:
    /*
    - Create an empty Buffer holding nullptr
    */
    Buffer();

    /*
    - Create a Buffer and copy buf in its internal buffer
    */
    Buffer(byte* buf, size_t length);

    /*
    - Create a Buffer and copy bufA then bufB in its internal buffer
    - the result is the concatenation of bufA and bufB.
    */
    Buffer(byte* bufA, size_t lengthA, byte* bufB, size_t lengthB);

    /*
    - Create a Buffer and hold internal pointer of buf, then invalid it.
    */
    Buffer(Buffer&& buf);

    /*
    - Create a Buffer and copy internal buffer of buf.
    */
    Buffer(Buffer& buf);

    /*
    - free internal buffer then destroy Buffer.
    */
    ~Buffer();


    /*
    - return a new Buffer that is the concatenation of this and b
    */
    Buffer  operator+(Buffer& b);
    Buffer  operator+(Buffer&& b);

    /*
    - steal internal buffer of b
    */
    void    operator=(Buffer&& b);

    /*
    - copy internal buffer of b
    */
    void    operator=(Buffer& buf);

    /*
    - return index-th byte of buffer
    */
    byte    operator[](uint16_t index);

    /*
    - return the first message of Buffer then remove it from Buffer
    */
    Message getMessage();

    /*
    - return internal buffer pointer
    */
    byte*   getPtr();

    /*
    - return size of internal buffer
    */
    size_t  getSize();

    /*
    - set internal buffer pointer to nullptr WITHOUT freeing it
    */
    void    invalid();

private:
    byte*   fBuffer;
    size_t  fLength;
};

#endif