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
    uint8_t AddSetting(Setting::Type type, void* data_ptr, size_t data_size, const char* name = "sans nom", void (*callback)() = nullptr);
    void UpdateSetting(uint8_t ref, byte* newValuePtr, size_t newValueSize);
    void SendUpdateMessage(Setting* setting);
    void SendUpdateMessage(uint8_t ref);
    
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

/**************** HELPER ********************/

#define INIT_WS_WITH_HTTP_SERVER_STR    \#include<HTTPServer.h>\
                                        #include<WebSocketCommunicator.h>\
                                        \
                                        STR::StartWiFi();\
                                        \
                                        HTTPServer *HTTPSERVER = new HTTPServer(8080);\
                                        STR* SETTINGATOR = new STR(WebSocketCommunicator::CreateInstance());

#define INIT_DEFAULT_SETTINGATOR          INIT_WS_WITH_HTTP_SERVER_STR

#endif