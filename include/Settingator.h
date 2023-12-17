#ifndef _SETTINGATOR_
#define _SETTINGATOR_

#include <Arduino.h>
#include "Communicator.h"
#include "Setting.h"
#include <vector>

class STR
{
    public:

    static void StartWiFi();

    STR(ICTR* communicator);

    void Update();
    void AddSetting(Setting& setting);
    void AddSetting(Setting::Type type, void* data_ptr, size_t data_size, const char* name = "sans nom", void (*callback)() = nullptr);

    Setting* GetSettingByRef(uint8_t ref);

    private:

    ICTR*                   fCommunicator = nullptr;
    uint8_t                 fInternalRefCount = 0;
    std::vector<Setting>    fSettingVector;
    
    Message*    _buildSettingInitMessage();
    
};

#endif