#ifndef _SETTINGATOR_
#define _SETTINGATOR_

#define STR_VERSION 0x035

#include <vector>

#if defined(ARDUINO)
#include <Arduino.h>

#elif defined(ESP_PLATFORM)


#endif

#include "Setting.h"

class Preferences;

class ICTR;
class Message;
class CTRBridge;
class CRGB;

struct notifCallback
{
    notifCallback(void(*inCallback)(), uint8_t inNotifByte);
    
    void(*callback)() = nullptr;
    uint8_t notifByte = 0;
};

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
    void UpdateSetting(uint8_t ref, uint8_t* newValuePtr, size_t newValueSize);
    void SendUpdateMessage(Setting* setting);
    void SendUpdateMessage(uint8_t ref);
    void SendNotif(uint8_t notifByte);
    void SendDirectNotif(uint8_t notifByte);
    void SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value = nullptr, uint8_t valueLen = 0);
    void AddNotifCallback(void(*callback)(), uint8_t notifByte);
    void SetCommunicator(ICTR* communicator);
    void StartEspNowInitBroadcasted();
    void StopEspNowInitBroadcasted();
    void InitNetworkHID(CRGB* led);
    void SetNetLed(uint8_t r, uint8_t g, uint8_t b);
    
    Setting*    GetSettingByRef(uint8_t ref);

    void        SavePreferences();
    void        begin();

    setting_ref settingRefCount();

    private:

    uint8_t                 fInternalRefCount = 0;
    std::vector<Setting>    fSettingVector;
    
    void        _sendInitMessage();
    void        _treatSettingUpdateMessage(Message* msg);
    void        _configEspNowDirectNotif(Message* msg);
    void        _configEspNowDirectSettingUpdate(Message* msg);
    void        _treatNotifMessage(Message* msg);
    void        _removeDirectNotifConfig(Message* msg);
    void        _removeDirectSettingUpdateConfig(Message* msg);

    Message*    _buildSettingInitMessage();
    
    Preferences*            fPreferences;

    uint8_t*                fSlaveID = nullptr;

    void        _createSlaveID(uint8_t slaveID);

    std::vector<notifCallback*> fNotifCallback;

    CTRBridge*  fBridge = nullptr;

    uint8_t     fBroadcastButtonPin = 32;
    uint8_t     fBridgeActivationButtonPin = 33;
    CRGB*       fInfoLED = nullptr;
};

extern Settingator STR;

#endif