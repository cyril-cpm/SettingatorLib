#include "Settingator.h"

#include "Communicator.h"
#include "CommunicatorBridge.h"
#include "Setting.h"
#include "Message.h"
#include "MiscDef.h"
#include "ESPNowCommunicator.h"
#include "CommandHandler.h"

#if defined(ARDUINO)
#include <WiFi.h>
#include <Preferences.h>
#include <FastLED.h>

#elif defined(ESP_PLATFORM)
#include <cstring>
#include <esp_log.h>
#include "esp_task_wdt.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "Led.h"
#endif


void Settingator::StartWiFi()
{

#if defined(ARDUINO)
    Serial.println("Begin WiFi");
    WiFi.mode(WIFI_AP);

    WiFi.softAPConfig(IPAddress(192,168,0,1), IPAddress(192,168,0,1), IPAddress(255,255,255,0));
    WiFi.softAP("espTest", "123456789");
    Serial.println(WiFi.softAPIP());

#elif defined(ESP_PLATFORM)


#endif
}

Settingator::Settingator(ICTR* communicator)
{
    masterCTR = communicator;
}

Settingator::~Settingator()
{

#if defined(ARDUINO)
    delete fPreferences;
#endif
}

void Settingator::SetNetLed(uint8_t r, uint8_t g, uint8_t b)
{
#if defined(ARDUINO)
    if (fInfoLED)
        *fInfoLED = CRGB(r, g, b);
#elif defined(ESP_PLATFORM)
    if (fInfoLED)
        *fInfoLED = RGB(r, g, b);
#endif
}

void Settingator::ESPNowBroadcastPing()
{
    if (xPortInIsrContext())
        fShouldESPNowBroadcastPing = true;
    else
        ESPNowCore::CreateInstance()->BroadcastPing();
}

#if defined(STR_BRIDGE_HID)
#define DEBOUNCE_TIME_MS 250

static esp_timer_handle_t debounceTimerBroadcast;

static void debounceTimerBroadcastCallback(void*)
{
    gpio_intr_enable(BROADCAST_PIN);
}

static esp_timer_handle_t debounceTimerBridgeActivation;

static void debounceTimerBridgeActivationCallback(void*)
{
    gpio_intr_enable(BRIDGE_ACTIVATION_PIN);
}

static void IRAM_ATTR broadcastInterruptHandler(void* arg)
{
    gpio_intr_disable(BROADCAST_PIN);

    esp_timer_start_once(debounceTimerBroadcast, DEBOUNCE_TIME_MS * 1000);
    STR.ESPNowBroadcastPing();
}


static void IRAM_ATTR bridgeActivationInterruptHandler(void* arg)
{
    gpio_intr_disable(BRIDGE_ACTIVATION_PIN);

    esp_timer_start_once(debounceTimerBridgeActivation, DEBOUNCE_TIME_MS * 1000);
    if (initEspNowBroadcasted)
        STR.StopEspNowInitBroadcasted();
    else
        STR.StartEspNowInitBroadcasted();
}
#else
#pragma message("No Bridge HID")
#endif

void Settingator::InitNetworkHID()
{
#if defined(STR_BRIDGE_HID)
#if defined (ARDUINO)
    pinMode(fBridgeActivationButtonPin, INPUT_PULLDOWN);
    attachInterrupt(fBridgeActivationButtonPin, [](){
        if (initEspNowBroadcasted)
        {
            Serial.println("Stop");
            STR.StopEspNowInitBroadcasted();
            STR.SetNetLed(255, 0, 0);
        }
        else
        {
            STR.StartEspNowInitBroadcasted();
            STR.SetNetLed(0, 255, 0);
            Serial.println("Start");
        }
    }, RISING);
    fInfoLED = led;
#endif
    // TIMER //
    esp_timer_create_args_t timer_args = {
        .callback = debounceTimerBroadcastCallback,
        .name = "debounce_timer_broadcast"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &debounceTimerBroadcast));

    esp_timer_create_args_t timer_args2 = {
        .callback = debounceTimerBridgeActivationCallback,
        .name = "debounce_timer_bridge_activation"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args2, &debounceTimerBridgeActivation));

    // GPIO CONFIG //
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << BROADCAST_PIN;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_config_t io_conf2 = {};
    io_conf2.intr_type = GPIO_INTR_DISABLE;
    io_conf2.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf2.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf2.mode = GPIO_MODE_INPUT;
    io_conf2.pin_bit_mask = 1ULL << BRIDGE_ACTIVATION_PIN;
    ESP_ERROR_CHECK(gpio_config(&io_conf2));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    ESP_ERROR_CHECK(gpio_isr_handler_add(BROADCAST_PIN, broadcastInterruptHandler, (void*)BROADCAST_PIN));
    ESP_ERROR_CHECK(gpio_set_intr_type(BROADCAST_PIN, GPIO_INTR_POSEDGE));

    ESP_ERROR_CHECK(gpio_isr_handler_add(BRIDGE_ACTIVATION_PIN, bridgeActivationInterruptHandler, NULL));
    ESP_ERROR_CHECK(gpio_set_intr_type(BRIDGE_ACTIVATION_PIN, GPIO_INTR_POSEDGE));
    fInfoLED = new RGB(0, 255, 0);
    fInfoLEDStrip = new Strip(NET_HID_LED_PIN, fInfoLED, 1);
#endif
}

void Settingator::SetCommunicator(ICTR* communicator)
{
    masterCTR = communicator;
}

void Settingator::Update()
{
    if (masterCTR)
    {
        Message* msg = nullptr;
        
        if (masterCTR->Available())
            msg = masterCTR->Read();
        //Serial.println("Message available");
        //printBuffer(msg->GetBufPtr(), msg->GetLength(), HEX);
        //Serial.println("");

        if (msg)
#if defined(ARDUINO)
            Serial.println("Message Read");
#elif defined(ESP_PLATFORM)
            ESP_LOGI("STR","Message read");
#endif

        if (!fSlaveID && msg && msg->GetType() == Message::Type::InitRequest)
            _createSlaveID(msg->GetSlaveID());

        if (fSlaveID && msg && *fSlaveID == msg->GetSlaveID())
        {
            auto msgType = msg->GetType();

            switch (msgType)
            {
            case Message::Type::InitRequest:
                _sendInitMessage();
                break;
            
            case Message::Type::SettingUpdate:
                _treatSettingUpdateMessage(msg);
                break;

            case Message::Type::ConfigEspNowDirectNotif:
                _configEspNowDirectNotif(msg);
                break;

            case Message::Type::ConfigEspNowDirectSettingUpdate:
                _configEspNowDirectSettingUpdate(msg);
                break;

            case Message::Type::Notif:
                _treatNotifMessage(msg);
                break;

            case Message::Type::RemoveDirectNotifConfig:
                _removeDirectNotifConfig(msg);
                break;

            case Message::Type::RemoveDirectSettingUpdateConfig:
                _removeDirectSettingUpdateConfig(msg);
                break;

            case Message::Type::Command:
                _treatCommandMessage(msg);
                break;
            default:
                //DEBUG_PRINT_VALUE_BUF_LN("UNTREATED MESSAGE", msg->GetBufPtr(), msg->GetLength())
                break;
            }
            masterCTR->Flush();
        }
        else
        {
            if (fBridge)
                fBridge->Update();
            else
                masterCTR->Flush();
        }
    }

#if defined(ESP_PLATFORM)
    ESP_ERROR_CHECK(esp_task_wdt_reset());
    vTaskDelay(1);
#endif

    if (fShouldStartEspNowInitBroadcasted)
    {
        StartEspNowInitBroadcasted();
        fShouldStartEspNowInitBroadcasted = false;
    }

    if (fShouldStopEspNowInitBroadcasted)
    {
        StopEspNowInitBroadcasted();
        fShouldStopEspNowInitBroadcasted = false;
    }

    if (fShouldESPNowBroadcastPing)
    {
        ESPNowBroadcastPing();
        fShouldESPNowBroadcastPing = false;
    }

    if (fInfoLEDStrip)
        fInfoLEDStrip->Show();
}

void Settingator::AddSetting(Setting& setting)
{
    fSettingVector.push_back(setting);

#if defined(ARDUINO)
    if (setting.getType() != Setting::Type::Trigger)
    {
        const char * settingName = setting.getName().c_str();
        void* buf = malloc(setting.getDataSize() * sizeof(uint8_t));
    
        if (fPreferences)
        {
            size_t len = fPreferences->getBytes(settingName, buf, setting.getDataSize());

            if (len)
                setting.update((uint8_t*)buf, len);
        }
    }
#endif
}

uint8_t Settingator::AddSetting(Setting::Type type, void* data_ptr, size_t data_size, const char* name, std::function<void()> callback)
{
    fSettingVector.push_back(Setting(type, data_ptr, data_size, name, callback, fInternalRefCount++));

    /*if (type != Setting::Type::Trigger)
    {
        void* buf = malloc(data_size * sizeof(uint8_t));
        DEBUG_PRINT_LN(name)
        size_t len = fPreferences->getBytes(name, buf, data_size);

        Setting* setting = GetSettingByRef(fInternalRefCount-1);

        if (setting && len)
        {
            DEBUG_PRINT_LN("Update Setting With preferences")
            setting->update((uint8_t*)buf, len);
            DEBUG_PRINT_VALUE_BUF_LN(name, (uint8_t*)buf, len)
        }
    }*/

    return(fInternalRefCount-1);
}

void Settingator::UpdateSetting(uint8_t ref, uint8_t* newValuePtr, size_t newValueSize)
{
    Setting* setting = GetSettingByRef(ref);

    if (setting)
    {
        setting->update(newValuePtr, newValueSize);
        SendUpdateMessage(setting);
    }
}

void Settingator::SendUpdateMessage(Setting* setting)
{
    if (setting)
    {
        Message* message = setting->buildUpdateMessage(fSlaveID);
        if (message && masterCTR)
            masterCTR->Write(*message);
        delete message;
    }
}

void Settingator::SendUpdateMessage(uint8_t ref)
{
    Setting* setting = GetSettingByRef(ref);

    if (setting)
        SendUpdateMessage(setting);
}

void Settingator::SendNotif(uint8_t notifByte)
{
    if (masterCTR)
    {
        size_t notifMsgSize = 7;

        uint8_t* notifBuffer = (uint8_t*)malloc(notifMsgSize * sizeof(uint8_t));

        notifBuffer[0] = Message::Frame::Start;
        notifBuffer[1] = 0;
        notifBuffer[2] = notifMsgSize;
        notifBuffer[3] = *fSlaveID;
        notifBuffer[4] = Message::Type::Notif;
        notifBuffer[5] = notifByte;
        notifBuffer[6] = Message::Frame::End;

        Message message(notifBuffer, notifMsgSize);

        masterCTR->Write(message);
    }
}

void Settingator::SendDirectNotif(uint8_t notifByte)
{
    if (masterCTR)
        masterCTR->SendDirectNotif(notifByte);
}

void Settingator::SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen)
{
    if (masterCTR)
        masterCTR->SendDirectSettingUpdate(settingRef, value, valueLen);
}

void Settingator::AddNotifCallback(void(*callback)(), uint8_t notifByte)
{
    fNotifCallback.push_back(new notifCallback(callback, notifByte));
}

Message* Settingator::_buildSettingInitMessage()
{
    size_t initRequestSize = 7;

    DEBUG_PRINT_LN("build setting init message")

    for (auto i = fSettingVector.begin(); i != fSettingVector.end(); i++)
    {
        initRequestSize += i->getInitRequestSize();
    }

    uint8_t* requestBuffer = (uint8_t*)malloc(initRequestSize * sizeof(uint8_t));

    requestBuffer[0] = Message::Frame::Start;
    requestBuffer[1] = initRequestSize >> 8;
    requestBuffer[2] = initRequestSize;
    requestBuffer[3] = *fSlaveID;
    requestBuffer[4] = Message::Type::SettingInit;
    requestBuffer[5] = fSettingVector.size();

    uint8_t* msgIndex = requestBuffer + 6;

    for (auto i = fSettingVector.begin(); i != fSettingVector.end(); i++)
    {
        i->getInitRequest(msgIndex);
        msgIndex += i ->getInitRequestSize();
    }

    requestBuffer[initRequestSize - 1] = Message::Frame::End;

    DEBUG_PRINT_VALUE("initRequestSize", initRequestSize)

    return new Message(requestBuffer, initRequestSize);
}

Setting* Settingator::GetSettingByRef(uint8_t ref)
{
    DEBUG_PRINT_VALUE("Searching setting ref", ref);
    for (auto it = fSettingVector.begin(); it != fSettingVector.end(); it++)
    {
        if (it->getRef() == ref)
        {
            DEBUG_PRINT_LN("FOUND");
            return &(*it);
        }
    }
    DEBUG_PRINT("NOT found");
    return nullptr;
}

void Settingator::SavePreferences()
{
#if defined(ARDUINO)
    if (fPreferences)
    {
        DEBUG_PRINT_LN("Saving Preferences");
        for (auto i = fSettingVector.begin(); i != fSettingVector.end(); i++)
        {
            if (i->getType() != Setting::Type::Trigger)
            {
                fPreferences->putBytes(i->getName().c_str(), i->getDataPtr(), i->getDataSize());
                DEBUG_PRINT_VALUE_BUF_LN(i->getName().c_str(), (uint8_t*)i->getDataPtr(), i->getDataSize())
            }
        }
    }
#endif
}

void Settingator::StartEspNowInitBroadcasted()
{
    if (xPortInIsrContext())
    {
        fShouldStartEspNowInitBroadcasted = true;
    }
    else
    {
        if (!fBridge)
            fBridge = CTRBridge::CreateInstance(masterCTR);

        if (fBridge)
        {
            fBridge->StartEspNowInitBroadcasted();
            SetNetLed(0, 0, 255);
        }
    }
}

void Settingator::StopEspNowInitBroadcasted()
{
    if (xPortInIsrContext())
    {
        fShouldStopEspNowInitBroadcasted = true;
    }
    else
    {
        if (fBridge)
        {
            fBridge->StopEspNowInitBroadcasted();
            SetNetLed(0, 255, 0);
        }
    }
}

void Settingator::begin()
{
#if defined(ARDUINO)
    if (fPreferences)
        fPreferences->end();
#endif

#if defined(ARDUINO)
    fPreferences = new Preferences();
    //fPreferences->begin("settingator", false);

#elif defined(ESP_PLATFORM)
    if (esp_task_wdt_status(nullptr) == ESP_ERR_NOT_FOUND)
        ESP_ERROR_CHECK(esp_task_wdt_add(nullptr));
#endif

    InitNetworkHID();
}

void Settingator::_createSlaveID(uint8_t slaveID)
{
    fSlaveID = new uint8_t;
    if (fSlaveID)
        *fSlaveID = slaveID;
}

void Settingator::_sendInitMessage()
{
    if (masterCTR)
    {
        Message* initMessage = _buildSettingInitMessage();
        //DEBUG_PRINT_LN("Settingator::Update");

        masterCTR->Write(*initMessage);

        delete initMessage;
    }
}

void Settingator::_treatSettingUpdateMessage(Message* msg)
{
    if (!msg)
        return;

    uint8_t* value;
    uint8_t ref;
    uint8_t valueLen;

    msg->ExtractSettingUpdate(ref, valueLen, &value);

    Setting *setting = GetSettingByRef(ref);
    
    if (!setting)
    {
        //Serial.println("Setting Not found");
        //Serial.println(ref);
    }

    if (setting  && (valueLen == setting->getDataSize()))
    {
        //Serial.println("Attempt to memcpy");
        memcpy((void*)setting->getDataPtr(), value, valueLen);
        //Serial.println("Done");
    }
    else
    {
        //Serial.println("Value Len is 0")
    }

    if (setting)
        setting->callback();
}

void Settingator::_configEspNowDirectNotif(Message* msg)
{
    if (masterCTR)
    {
        auto buffer = msg->GetBufPtr();
        masterCTR->ConfigEspNowDirectNotif(&buffer[6], buffer[16], buffer[5]);
    }
}

void Settingator::_configEspNowDirectSettingUpdate(Message* msg)
{
    if (masterCTR)
    {
        DEBUG_PRINT_LN("Config")
        auto buffer = msg->GetBufPtr();
        masterCTR->ConfigEspNowDirectSettingUpdate(&buffer[6], buffer[12], buffer[13], buffer[5]);
    }
}

void Settingator::_treatNotifMessage(Message* msg)
{
    if (!msg)
        return;

    auto buffer = msg->GetBufPtr();
    auto notifByte = buffer[5];

    for (auto i = fNotifCallback.begin(); i != fNotifCallback.end(); i++)
    {
        if ((*i)->notifByte == notifByte)
            (*i)->callback();
    }
}

void Settingator::_removeDirectNotifConfig(Message* msg)
{
    if (!msg || !masterCTR)
        return;

    auto buffer = msg->GetBufPtr();
    masterCTR->RemoveDirectNotifConfig(buffer[5], buffer[6]);
}

void Settingator::_removeDirectSettingUpdateConfig(Message* msg)
{
    if (!msg || !masterCTR)
        return;
        
    auto buffer = msg->GetBufPtr();
    masterCTR->RemoveDirectSettingUpdateConfig(buffer[5], buffer[6]);
}

void Settingator::_treatCommandMessage(Message *msg)
{
    if (!msg || !masterCTR)
        return;

    auto buffer = msg->GetBufPtr();

    char* cmdBuffer = (char*)&(buffer[4]);

    if (fCommandHandler)
        fCommandHandler->TreatCommand(cmdBuffer);
}

setting_ref Settingator::settingRefCount()
{
    return fInternalRefCount++;
}

//Settingator STR(nullptr);