#include "Definitions.h"

#include "STR.h"
#include "Setting.h"
#include "MiscDef.h"
#include "Message.h"
#include <cstdint>
#include <cstring>

Setting::Setting(Type type, void* dataPtr, size_t dataSize, const char* name, std::function<void()> callback, setting_ref ref)
: fType(type), fDataPtr((uint8_t*)dataPtr), fDataSize(dataSize), /*fName(name),*/ fRef(ref), fCallback(callback)
{
#if SERIAL_DEBUG
    /*Serial.println("New Setting:");
    Serial.print("\ttype\t\t: ");
    Serial.println(type, DEC);
    Serial.print("\tdataSize\t: ");
    Serial.println(dataSize, DEC);
    Serial.print("\tdata\t\t: ");
    printBuffer(fDataPtr, dataSize);
    Serial.println("");
    Serial.print("\tref\t\t: ");
    Serial.println(ref);
    Serial.print("\tname\t\t: ");
    Serial.println(name);*/
#endif
}

bool Setting::update(uint8_t* newValuePtr, size_t newValueSize)
{
    if (newValueSize > fDataSize && newValuePtr != nullptr)
        return false;

    memcpy(fDataPtr, newValuePtr, newValueSize);
    return true;
}

void Setting::getInitRequest(uint16_t index) const
{
    messageBuffer[index] = fRef;
    messageBuffer[index + 1] = fType;
    messageBuffer[index + 2] = fDataSize;
    memcpy(messageBuffer.data() + index + 3, fDataPtr, fDataSize);
    // messageBuffer[index + 3 + fDataSize] = fName.length();
    uint8_t nameIndex = index + 4 + fDataSize;

    // memcpy(messageBuffer.data() + nameIndex, fName.data(), fName.length());
}

uint16_t Setting::getInitRequestSize() const
{
    size_t bufferSize = 0;

    bufferSize += 4;
    // bufferSize += fName.length();
    bufferSize += fDataSize;

    return bufferSize;
}

void Setting::setCallback(void (*callback)())
{
    fCallback = callback;
}
