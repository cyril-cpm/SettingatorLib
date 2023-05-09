#include "Setting.h"

Setting::Setting(Type type, byte* dataPtr, size_t dataSize, String& name, setting_ref ref)
: fType(type), fDataPtr(dataPtr), fDataSize(dataSize), fName(name), fRef(ref)
{}

bool Setting::update(byte* newValuePtr, size_t newValueSize)
{
    if (newValueSize > fDataSize && newValuePtr != nullptr)
        return false;

    memcpy(fDataPtr, newValuePtr, newValueSize);
    return true;
}

void Setting::getInitRequest(byte** initRequestBuffer, size_t& bufferSize)
{
    bufferSize = 0;

    bufferSize += 4;
    bufferSize += fName.length();
    bufferSize += fDataSize;

    byte* buffer = (byte*)malloc(fDataSize);

    buffer[0] = fRef;
    buffer[1] = fType;
    buffer[2] = fDataSize;
    memcpy(&(buffer[3]), fDataPtr, fDataSize);
    buffer[3 + fDataSize] = fName.length();
    fName.getBytes(buffer, bufferSize, 4 + fDataSize);

    *initRequestBuffer = buffer;
}