#ifndef _SETTINGATOR_
#define _SETTINGATOR_

#include <Arduino.h>
#include "Communicator.h"
#include "Setting.h"

class STR
{
    public:

    STR(ICTR* communicator);

    void Update();
    void AddSetting(Setting::Type type, byte* data_ptr, size_t data_size, const char* name = "sans nom");

    private:

    ICTR*       fCommunicator = nullptr;
    uint8_t    fInternalRefCount = 0;
    
    Message*    _buildSettingInitMessage();
    
};

#endif