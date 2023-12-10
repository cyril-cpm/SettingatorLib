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

            fCommunicator->Write(*initMessage);

            delete initMessage;
        }

        fCommunicator->Flush();
    }
}

void STR::AddSetting(Setting& setting)
{
    fSettingVector.push_back(setting);
}

void STR::AddSetting(Setting::Type type, byte* data_ptr, size_t data_size, const char* name)
{
    fSettingVector.push_back(Setting(type, data_ptr, data_size, name, fInternalRefCount++));
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