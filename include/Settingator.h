#ifndef _SETTINGATOR_
#define _SETTINGATOR_

#define STR_VERSION 0x033

#include <vector>
#include <Arduino.h>

#include "Setting.h"

class Preferences;

class ICTR;
class Message;

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
    void UpdateSetting(uint8_t ref, byte* newValuePtr, size_t newValueSize);
    void SendUpdateMessage(Setting* setting);
    void SendUpdateMessage(uint8_t ref);
    void SendNotif(uint8_t notifByte);
    void SendDirectNotif(uint8_t notifByte);
    void SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value = nullptr, uint8_t valueLen = 0);
    void AddNotifCallback(void(*callback)(), uint8_t notifByte);
    void SetCommunicator(ICTR* communicator);
    
    Setting*    GetSettingByRef(uint8_t ref);

    void        SavePreferences();
    void        begin();

    setting_ref settingRefCount();

    private:

    ICTR*                   fCommunicator = nullptr;
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
};

extern Settingator STR;

#endif