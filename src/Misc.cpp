#include "MiscDef.h"


void printBuffer(byte* ptr, size_t size, uint8_t base)
{
    for (size_t i = 0; i < size; i++)
    {
        Serial.print(ptr[i], base);
        Serial.print(" ");
    }
}