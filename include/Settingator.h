#ifndef _SETTINGATOR_
#define _SETTINGATOR_

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FS.h>
#include <SPIFFS.h>
#include "Communicator.h"
#include "Setting.h"
#include <vector>
#include <Preferences.h>

class STR
{
    public:

    static void StartWiFi();

    STR(ICTR* communicator);

    void Update();
    void AddSetting(Setting& setting);
    void AddSetting(Setting::Type type, void* data_ptr, size_t data_size, const char* name = "sans nom", void (*callback)() = nullptr);

    Setting*    GetSettingByRef(uint8_t ref);

    void        SavePreferences();
    void        begin();

    private:

    ICTR*                   fCommunicator = nullptr;
    uint8_t                 fInternalRefCount = 0;
    std::vector<Setting>    fSettingVector;
    
    Message*    _buildSettingInitMessage();
    
    Preferences             fPreferences;
};

#endif