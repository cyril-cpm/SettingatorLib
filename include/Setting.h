#ifndef _SETTING_
#define _SETTING_

#include "MiscDef.h"

#include <Arduino.h>

typedef byte setting_ref;

class Setting
{
public:
    enum Type
    {
        Slider = 0x01,
        Trigger = 0x02,
        Switch = 0x03
    };

    Setting(Type type, void* dataPtr, size_t dataSize, const char* name, setting_ref ref);

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

    size_t getInitRequestSize();

private:
    Type fType;
    byte* fDataPtr = nullptr;
    size_t fDataSize = 0;
    String fName;
    setting_ref fRef = 0;
};

#endif