#pragma once

#include "Definitions.h"

#include <functional>
#include <vector>
#include <optional>

#include "Led.h"
#include "Setting.h"
#include "CommunicatorBridge.h"

#include "Core.h"

class Preferences;

class ICTR;
class Message;
class CTRBridge;
class CRGB;

struct notifCallback
{
	notifCallback() {}
    notifCallback(void(*inCallback)(), uint8_t inNotifByte) : callback(inCallback),
																notifByte(inNotifByte)
	{}
    
    void(*callback)() = nullptr;
    uint8_t notifByte = 0;
};

class Settingator
{
    public:

	static Settingator& GetInstance() {
		static Settingator instance;
		return instance;
	}

    static void StartWiFi();

    //static Settingator* CreateWSComWithHTTPServerWiFiSettingator();

    Settingator();
    ~Settingator();

    void Update();
    uint8_t AddSetting(Setting::Type type, void* data_ptr, size_t data_size, const char* name = "sans nom", std::function<void()> callback = nullptr);
    void UpdateSetting(uint8_t ref, uint8_t* newValuePtr, size_t newValueSize);
    void SendUpdateMessage(Setting& setting);
    void SendUpdateMessage(uint8_t ref);
    void SendNotif(uint8_t notifByte);
    void SendDirectNotif(uint8_t notifByte);
    void SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value = nullptr, uint8_t valueLen = 0);
    void AddNotifCallback(void(*callback)(), uint8_t notifByte);
    void StartEspNowInitBroadcasted();
    void StopEspNowInitBroadcasted();

#if defined(STR_BRIDGE_HID)
    void InitNetworkHID();
    void SetNetLed(uint8_t r, uint8_t g, uint8_t b);
#endif
    
	std::optional<std::reference_wrapper<Setting>>    GetSettingByRef(uint8_t ref);

    void        SavePreferences();
    void        begin();

    setting_ref settingRefCount();

    private:

    uint8_t                 fInternalRefCount = 0;
    std::array<std::optional<Setting>, CONFIG_STR_NB_SETTINGS>    fSettingArray;

    void        _sendInitMessage();

    void        _treatSettingUpdateMessage(const ICore& core);
    void        _treatNotifMessage(const ICore& core);

    void        _configEspNowDirectNotif(Message& msg);
    void        _configEspNowDirectSettingUpdate(Message& msg);

    void        _removeDirectNotifConfig(Message& msg);
    void        _removeDirectSettingUpdateConfig(Message& msg);

    Message    _buildSettingInitMessage();

    uint8_t                fSlaveID = 0;

    void        _createSlaveID(uint8_t slaveID);

    std::array<notifCallback, 4> fNotifCallback;

#if defined(STR_BRIDGE_HID)
    uint8_t     fBroadcastButtonPin = 32;
    uint8_t		fBridgeActivationButtonPin = 33;
    RGB			fInfoLED;
    Strip		fInfoLEDStrip;
#endif

    bool        fShouldStartEspNowInitBroadcasted = false;
    bool        fShouldStopEspNowInitBroadcasted = false;
    bool        fShouldESPNowBroadcastPing = false;
};
