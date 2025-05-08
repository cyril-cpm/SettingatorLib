#ifndef _SETTING_
#define _SETTING_

#ifdef ARDUINO
#include <Arduino.h>
#elif defined(ESP_PLATFORM)
#include <esp_types.h>
#endif

#include <string>

class Message;

typedef uint8_t setting_ref;

class Setting
{
public:
    enum Type
    {
        Slider = 0x01,
        Trigger = 0x02,
        Switch = 0x03,
        Label = 0x04,

        UInt32 = 0x20,
        UInt8 = 0x21,
        UInt16 = 0x22,
        Float = 0x23,
        Bool = 0x24,

        // TESTING PURPOSE ONLY
        CustomFloat = 0xFE
    };

    Setting(Type type, void* dataPtr, size_t dataSize, const char* name, void (*callback)(), setting_ref ref);

    /*
    - Update the setting value
    */
    bool update(uint8_t* newValuePtr, size_t newValueSize);

    /*
    - Build the init buffer describing the setting to be sent to the remote controller
    - in SETTING_INI message
    - initRequestBuffer is allocated and must be freed then.
    */
    void getInitRequest(uint8_t* initRequestBuffer);
    Message* buildUpdateMessage(uint8_t* slaveID);

    size_t getInitRequestSize();

    uint8_t getRef() const { return fRef; }
    size_t  getDataSize() const { return fDataSize; }
    uint8_t*   getDataPtr() { return fDataPtr; }
    Type    getType() const { return fType; }

    void    callback() { if (fCallback) fCallback(); }
    void    setCallback(void (*callback)());

    std::string  getName() { return fName; }

private:
    Type fType;
    uint8_t* fDataPtr = nullptr;
    size_t fDataSize = 0;
    std::string fName;
    setting_ref fRef = 0;
    void    (*fCallback)();
};

#endif