#include "Settingator.h"
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

void STR::StartWiFi()
{
    Serial.println("Begin WiFi");
    WiFi.mode(WIFI_AP);

    WiFi.softAPConfig(IPAddress(192,168,0,1), IPAddress(192,168,0,1), IPAddress(255,255,255,0));
    WiFi.softAP("espTest", "123456788");
    Serial.println(WiFi.softAPIP());
}

STR::STR(ICTR* communicator) : fCommunicator(communicator)
{
    
}

void STR::Update()
{
    if (fCommunicator->Available())
    {
        Message* msg = fCommunicator->Read();
        Serial.println("Message available");
        printBuffer(msg->GetBufPtr(), msg->GetLength(), HEX);
        Serial.println("");

    
        if (msg->GetType() == Message::Type::InitRequest)
        {
            Message* initMessage = _buildSettingInitMessage();
    //DEBUG_PRINT_LN("STR::Update");

            fCommunicator->Write(*initMessage);

            delete initMessage;
        }
        else if (msg->GetType() == Message::Type::SettingUpdate)
        {
            Serial.println("Setting update Message");
            byte* value;
            uint8_t ref;
            uint8_t valueLen;

            msg->ExtractSettingUpdate(ref, valueLen, &value);

            Setting *setting = GetSettingByRef(ref);

            if (setting  && (valueLen == setting->getDataSize()))
            {
                Serial.println("Attempt to memcpy");
                memcpy((void*)setting->getDataPtr(), value, valueLen);
                Serial.println("Done");
            }

            if (setting)
                setting->callback();
        }

        fCommunicator->Flush();
    }
}

void STR::AddSetting(Setting& setting)
{
    fSettingVector.push_back(setting);
}

void STR::AddSetting(Setting::Type type, void* data_ptr, size_t data_size, const char* name, void (*callback)())
{
    fSettingVector.push_back(Setting(type, data_ptr, data_size, name, callback, fInternalRefCount++));
}

Message* STR::_buildSettingInitMessage()
{
    size_t initRequestSize = 6;

    for (auto i = fSettingVector.begin(); i != fSettingVector.end(); i++)
    {
        initRequestSize += i->getInitRequestSize();
    }

    byte* requestBuffer = (byte*)malloc(initRequestSize * sizeof(byte));

    requestBuffer[0] = Message::Frame::Start;
    requestBuffer[1] = initRequestSize >> 8;
    requestBuffer[2] = initRequestSize;
    requestBuffer[3] = Message::Type::SettingInit;
    requestBuffer[4] = fSettingVector.size();

    byte* msgIndex = requestBuffer + 5;

    for (auto i = fSettingVector.begin(); i != fSettingVector.end(); i++)
    {
        i->getInitRequest(msgIndex);
        msgIndex += i ->getInitRequestSize();
    }

    requestBuffer[initRequestSize - 1] = Message::Frame::End;

    return new Message(requestBuffer, initRequestSize);
}

Setting* STR::GetSettingByRef(uint8_t ref)
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