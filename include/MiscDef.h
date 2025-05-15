#ifndef _MISCDEF_
#define _MISCDEF_

#if defined(ARDUINO)
#include <Arduino.h>

#elif defined(ESP_PLATFORM)
#include <esp_types.h>
#define HEX 16
#endif

#define SERIAL_DEBUG 1

#if SERIAL_DEBUG

#define DEBUG(X) X

#else

#define DEBUG(X)

#endif

void printBuffer(const uint8_t* ptr, size_t size, uint8_t base = HEX);

#define DEBUG_PRINT_LN(X) DEBUG(/*Serial.println(X);*/)

#define DEBUG_PRINT(X) DEBUG(/*Serial.print(X);*/)

#define DEBUG_PRINT_VALUE(X, Y) DEBUG(/*Serial.print(X);Serial.print(" : ");Serial.println(Y);*/)

#define DEBUG_PRINT_BUF(PTR, SIZE) DEBUG(printBuffer(PTR, SIZE);)

#define DEBUG_PRINT_BUF_LN(PTR, SIZE) DEBUG_PRINT_BUF(PTR, SIZE) DEBUG(Serial.println();)

#define DEBUG_PRINT_VALUE_BUF(NAME, PTR, SIZE) DEBUG_PRINT(NAME) DEBUG_PRINT(" : ") DEBUG_PRINT_BUF(PTR, SIZE)

#define DEBUG_PRINT_VALUE_BUF_LN(NAME, PTR, SIZE) DEBUG_PRINT_VALUE_BUF(NAME, PTR, SIZE) DEBUG_PRINT_LN("")

#endif