#include "Setting.h"
#include "MiscDef.h"
#include <Arduino.h>

Setting::Setting(Type type, void* dataPtr, size_t dataSize, const char* name, setting_ref ref)
: fType(type), fDataPtr((byte*)dataPtr), fDataSize(dataSize), fName(name), fRef(ref)
{
#if SERIAL_DEBUG
    Serial.println("New Setting:");
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
    Serial.println(name);
#endif
}

bool Setting::update(byte* newValuePtr, size_t newValueSize)
{
    if (newValueSize > fDataSize && newValuePtr != nullptr)
        return false;

    memcpy(fDataPtr, newValuePtr, newValueSize);
    return true;
}

void Setting::getInitRequest(byte* buffer)
{
    size_t bufferSize = getInitRequestSize();

    bufferSize += 4;
    bufferSize += fName.length();
    bufferSize += fDataSize;

    buffer[0] = fRef;
    buffer[1] = fType;
    buffer[2] = fDataSize;
    memcpy(&(buffer[3]), fDataPtr, fDataSize);
    buffer[3 + fDataSize] = fName.length();
    byte nameIndex = 4 + fDataSize;
    fName.getBytes(buffer + 4 + fDataSize, bufferSize - nameIndex + 1, 0);
}

size_t Setting::getInitRequestSize()
{
    size_t bufferSize = 0;

    bufferSize += 4;
    bufferSize += fName.length();
    bufferSize += fDataSize;

    return bufferSize;
}