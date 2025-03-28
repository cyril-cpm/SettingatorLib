#ifndef _SETTING_
#define _SETTING_

#include <Arduino.h>

class Message;

typedef byte setting_ref;

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
    bool update(byte* newValuePtr, size_t newValueSize);

    /*
    - Build the init buffer describing the setting to be sent to the remote controller
    - in SETTING_INI message
    - initRequestBuffer is allocated and must be freed then.
    */
    void getInitRequest(byte* initRequestBuffer);
    Message* buildUpdateMessage(uint8_t* slaveID);

    size_t getInitRequestSize();

    uint8_t getRef() { return fRef; } const
    size_t  getDataSize() { return fDataSize; } const
    byte*   getDataPtr() { return fDataPtr; }
    Type    getType() { return fType; } const

    void    callback() { if (fCallback) fCallback(); }
    void    setCallback(void (*callback)());

    String  getName() { return fName; }

private:
    Type fType;
    byte* fDataPtr = nullptr;
    size_t fDataSize = 0;
    String fName;
    setting_ref fRef = 0;
    void    (*fCallback)();
};

#endif