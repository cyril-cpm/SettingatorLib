#include "Settingator.h"

#include "Communicator.h"
#include "Setting.h"
#include "Message.h"
#include "MiscDef.h"

#include <WiFi.h>
#include <Preferences.h>


void Settingator::StartWiFi()
{
    Serial.println("Begin WiFi");
    WiFi.mode(WIFI_AP);

    WiFi.softAPConfig(IPAddress(192,168,0,1), IPAddress(192,168,0,1), IPAddress(255,255,255,0));
    WiFi.softAP("espTest", "123456789");
    Serial.println(WiFi.softAPIP());
}

Settingator::Settingator(ICTR* communicator) : fCommunicator(communicator)
{
    fPreferences = new Preferences();
    //fPreferences->begin("settingator", false);
}

Settingator::~Settingator()
{
    delete fPreferences;
}

void Settingator::SetCommunicator(ICTR* communicator)
{
    fCommunicator = communicator;
}

void Settingator::Update()
{
    if (fCommunicator && fCommunicator->Available())
    {
        Message* msg = fCommunicator->Read();
        //Serial.println("Message available");
        //printBuffer(msg->GetBufPtr(), msg->GetLength(), HEX);
        //Serial.println("");

        if (!fSlaveID && msg && msg->GetType() == Message::Type::InitRequest)
            _createSlaveID(msg->GetSlaveID());

        if (fSlaveID && msg && *fSlaveID == msg->GetSlaveID())
        {
            auto msgType = msg->GetType();

            switch (msg->GetType())
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
            default:
                DEBUG_PRINT_VALUE_BUF_LN("UNTREATED MESSAGE", msg->GetBufPtr(), msg->GetLength())
                break;
            }
        }

        fCommunicator->Flush();
    }
}

void Settingator::AddSetting(Setting& setting)
{
    fSettingVector.push_back(setting);

    if (setting.getType() != Setting::Type::Trigger)
    {
        const char * settingName = setting.getName().c_str();
        void* buf = malloc(setting.getDataSize() * sizeof(byte));

        if (fPreferences)
        {
            size_t len = fPreferences->getBytes(settingName, buf, setting.getDataSize());

            if (len)
                setting.update((byte*)buf, len);
        }
    }
}

uint8_t Settingator::AddSetting(Setting::Type type, void* data_ptr, size_t data_size, const char* name, void (*callback)())
{
    fSettingVector.push_back(Setting(type, data_ptr, data_size, name, callback, fInternalRefCount++));

    /*if (type != Setting::Type::Trigger)
    {
        void* buf = malloc(data_size * sizeof(byte));
        DEBUG_PRINT_LN(name)
        size_t len = fPreferences->getBytes(name, buf, data_size);

        Setting* setting = GetSettingByRef(fInternalRefCount-1);

        if (setting && len)
        {
            DEBUG_PRINT_LN("Update Setting With preferences")
            setting->update((byte*)buf, len);
            DEBUG_PRINT_VALUE_BUF_LN(name, (byte*)buf, len)
        }
    }*/

    return(fInternalRefCount-1);
}

void Settingator::UpdateSetting(uint8_t ref, byte* newValuePtr, size_t newValueSize)
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
        if (message && fCommunicator)
            fCommunicator->Write(*message);
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
    if (fCommunicator)
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

        fCommunicator->Write(message);
    }
}

void Settingator::SendDirectNotif(uint8_t notifByte)
{
    if (fCommunicator)
        fCommunicator->SendDirectNotif(notifByte);
}

void Settingator::SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen)
{
    if (fCommunicator)
        fCommunicator->SendDirectSettingUpdate(settingRef, value, valueLen);
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

    byte* requestBuffer = (byte*)malloc(initRequestSize * sizeof(byte));

    requestBuffer[0] = Message::Frame::Start;
    requestBuffer[1] = initRequestSize >> 8;
    requestBuffer[2] = initRequestSize;
    requestBuffer[3] = *fSlaveID;
    requestBuffer[4] = Message::Type::SettingInit;
    requestBuffer[5] = fSettingVector.size();

    byte* msgIndex = requestBuffer + 6;

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
    if (fPreferences)
    {
        DEBUG_PRINT_LN("Saving Preferences");
        for (auto i = fSettingVector.begin(); i != fSettingVector.end(); i++)
        {
            if (i->getType() != Setting::Type::Trigger)
            {
                fPreferences->putBytes(i->getName().c_str(), i->getDataPtr(), i->getDataSize());
                DEBUG_PRINT_VALUE_BUF_LN(i->getName().c_str(), (byte*)i->getDataPtr(), i->getDataSize())
            }
        }
    }
}

void Settingator::begin()
{
    if (fPreferences)
        fPreferences->end();
}

void Settingator::_createSlaveID(uint8_t slaveID)
{
    fSlaveID = new uint8_t;
    if (fSlaveID)
        *fSlaveID = slaveID;
}

void Settingator::_sendInitMessage()
{
    if (fCommunicator)
    {
        Message* initMessage = _buildSettingInitMessage();
        //DEBUG_PRINT_LN("Settingator::Update");

        fCommunicator->Write(*initMessage);

        delete initMessage;
    }
}

void Settingator::_treatSettingUpdateMessage(Message* msg)
{
    if (!msg)
        return;

    byte* value;
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
        //Serial.println("Value Len is 0")
;
    if (setting)
        setting->callback();
}

void Settingator::_configEspNowDirectNotif(Message* msg)
{
    if (fCommunicator)
    {
        auto buffer = msg->GetBufPtr();
        fCommunicator->ConfigEspNowDirectNotif(&buffer[6], buffer[16], buffer[5]);
    }
}

void Settingator::_configEspNowDirectSettingUpdate(Message* msg)
{
    if (fCommunicator)
    {
        DEBUG_PRINT_LN("Config")
        auto buffer = msg->GetBufPtr();
        fCommunicator->ConfigEspNowDirectSettingUpdate(&buffer[6], buffer[12], buffer[13], buffer[5]);
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
        if ((*i)->notifByte = notifByte)
            (*i)->callback();
    }
}

void Settingator::_removeDirectNotifConfig(Message* msg)
{
    if (!msg || !fCommunicator)
        return;

    auto buffer = msg->GetBufPtr();
    fCommunicator->RemoveDirectNotifConfig(buffer[5], buffer[6]);
}

void Settingator::_removeDirectSettingUpdateConfig(Message* msg)
{
    if (!msg || !fCommunicator)
        return;
        
    auto buffer = msg->GetBufPtr();
    fCommunicator->RemoveDirectSettingUpdateConfig(buffer[5], buffer[6]);
}

setting_ref Settingator::settingRefCount()
{
    return fInternalRefCount++;
}

//Settingator STR(nullptr);