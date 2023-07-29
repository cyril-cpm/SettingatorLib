#ifndef _MISCDEF_
#define _MISCDEF_

#include <Arduino.h>

#define SERIAL_DEBUG 1

#if SERIAL_DEBUG
    
void printBuffer(byte* ptr, size_t size, uint8_t base = HEX);
#endif

#endif