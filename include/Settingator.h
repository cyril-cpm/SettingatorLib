#ifndef _SETTINGATOR_
#define _SETTINGATOR_

#define STR_VERSION 0x024

#include <vector>
#include <Arduino.h>

#include "Setting.h"

class Preferences;

class ICTR;
class Message;

class Settingator
{
    public:

    static void StartWiFi();

    //static Settingator* CreateWSComWithHTTPServerWiFiSettingator();

    Settingator(ICTR* communicator);
    ~Settingator();

    void Update();
    void AddSetting(Setting& setting);
    uint8_t AddSetting(Setting::Type type, void* data_ptr, size_t data_size, const char* name = "sans nom", void (*callback)() = nullptr);
    void UpdateSetting(uint8_t ref, byte* newValuePtr, size_t newValueSize);
    void SendUpdateMessage(Setting* setting);
    void SendUpdateMessage(uint8_t ref);
    void SetCommunicator(ICTR* communicator);
    
    Setting*    GetSettingByRef(uint8_t ref);

    void        SavePreferences();
    void        begin();

    private:

    ICTR*                   fCommunicator = nullptr;
    uint8_t                 fInternalRefCount = 0;
    std::vector<Setting>    fSettingVector;
    
    Message*    _buildSettingInitMessage();
    
    Preferences*            fPreferences;

    uint8_t*                fSlaveID = nullptr;

    void        _createSlaveID(uint8_t slaveID);
};

extern Settingator STR;

#endif