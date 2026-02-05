#include "Setting.h"
#include "MiscDef.h"
#include "Message.h"

#if defined(ARDUINO)

#elif defined(ESP_PLATFORM)
#include <cstring>

#endif

Setting::Setting(Type type, void* dataPtr, size_t dataSize, const char* name, std::function<void()> callback, setting_ref ref)
: fType(type), fDataPtr((uint8_t*)dataPtr), fDataSize(dataSize), fName(name), fRef(ref), fCallback(callback)
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

void Setting::getInitRequest(uint8_t* buffer)
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
    uint8_t nameIndex = 4 + fDataSize;

    memcpy(buffer + nameIndex, fName.data(), fName.length());
}

size_t Setting::getInitRequestSize()
{
    size_t bufferSize = 0;

    bufferSize += 4;
    bufferSize += fName.length();
    bufferSize += fDataSize;

    return bufferSize;
}

Message Setting::buildUpdateMessage(uint8_t* slaveID)
{
    size_t messageLength = 8 + fDataSize;

	std::vector<uint8_t> buffer(messageLength);

    buffer[0] = Message::Frame::Start;

    buffer[1] = messageLength >> 8;
    buffer[2] = messageLength;

    if (slaveID)
        buffer[3] = *slaveID;
    else
        buffer[3] = 0;

    buffer[4] = Message::Type::SettingUpdate;

    buffer[5] = fRef;
    buffer[6] = fDataSize;
    memcpy(&(buffer[7]), fDataPtr, fDataSize);
    buffer[messageLength - 1] = Message::Frame::End;

    return Message(std::move(buffer));
}

void Setting::setCallback(void (*callback)())
{
    fCallback = callback;
}
