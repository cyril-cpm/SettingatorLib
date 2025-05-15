#include "MiscDef.h"


void printBuffer(const uint8_t* ptr, size_t size, uint8_t base)
{
#if defined(ARDUINO)

#if SERIAL_DEBUG
    for (size_t i = 0; i < size; i++)
    {
        Serial.print(ptr[i], base);
        Serial.print(" ");
    }
#endif

#elif defined(ESP_PLATFORM)

#endif

}
