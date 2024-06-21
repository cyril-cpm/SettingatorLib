#include "ESPNowCommunicator.h"
#include "MiscDef.h"
#include "Message.h"
#include <esp_now.h>
#include <WiFi.h>
#include <mutex>

std::mutex espNowMsgListMutex;

std::queue<espNowMsg*> espNowMsgList;

espNowMsg::espNowMsg(const uint8_t* inData, int inLen) : len(inLen)
{
    data = (uint8_t*)malloc(sizeof(uint8_t) * len);
    if (data)
        memcpy(data, inData, len * sizeof(uint8_t));
}

espNowMsg::~espNowMsg()
{
    delete data;
}

ESPNowCTR*  ESPNowCTR::CreateInstanceDiscoverableWithSSID(const char* deviceName)
{
    WiFi.mode(WIFI_MODE_APSTA);
    WiFi.softAP(deviceName);
    DEBUG_PRINT(WiFi.macAddress());

    return new ESPNowCTR();
}

ESPNowCTR* ESPNowCTR::CreateInstanceWithMac(uint8_t* mac)
{
    WiFi.mode(WIFI_STA);
    DEBUG_PRINT(WiFi.macAddress());

    return new ESPNowCTR(mac);
}

static void receiveCallback(const uint8_t* mac, const uint8_t* inData, int len)
{
    esp_now_peer_info peerInfo;

    if (esp_now_get_peer(mac, &peerInfo) == ESP_ERR_ESPNOW_NOT_FOUND)
    {
        peerInfo = {};
        memcpy(peerInfo.peer_addr, mac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        esp_now_add_peer(&peerInfo);
    }

    espNowMsgListMutex.lock();
    espNowMsgList.push(new espNowMsg(inData, len));
    espNowMsgListMutex.unlock();
}

ESPNowCTR::ESPNowCTR(uint8_t* mac)
{
    esp_now_init();
    esp_now_register_recv_cb(receiveCallback);

    if (mac != nullptr)
    {
        fMac = (uint8_t*)malloc(6 * sizeof(uint8_t));
        memcpy(fMac, mac, 6 * sizeof(uint8_t));
        
        esp_now_peer_info peerInfo = {};

        memcpy(peerInfo.peer_addr, fMac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        esp_now_add_peer(&peerInfo);
    }
}

ESPNowCTR::~ESPNowCTR()
{
    esp_now_deinit();
}

void ESPNowCTR::Update()
{
    if (espNowMsgListMutex.try_lock())
    {
        while (espNowMsgList.size())
        {
            auto msg = espNowMsgList.front();

            _receive(new Message(msg->data, msg->len));

            delete msg;
            espNowMsgList.pop();
        }
        espNowMsgListMutex.unlock();
    }
}

int ESPNowCTR::Write(Message& buf)
{
    DEBUG_PRINT_LN(buf.GetLength())
    if (buf.GetLength() > 250)
        DEBUG_PRINT_LN("Alert ! message to big for ESPNow !")
    esp_now_send(fMac, buf.GetBufPtr(), buf.GetLength());
    return 0;
}

espNowDirectNotif::espNowDirectNotif(const uint8_t* inMac, uint8_t inNotifByte, uint8_t inDstSlaveID) : notifByte(inNotifByte), dstSlaveID(inDstSlaveID)
{
    mac = (uint8_t*)malloc(6);
    memcpy(mac, inMac, 6);
}

espNowDirectNotif::~espNowDirectNotif()
{
    delete mac;
}

espNowDirectSettingUpdate::espNowDirectSettingUpdate(const uint8_t* inMac, uint8_t inSettingRef, uint8_t inDstSlaveID, uint8_t inValueLen) : settingRef(inSettingRef), dstSlaveID(inDstSlaveID), valueLen(inValueLen)
{
    mac = (uint8_t*)malloc(6);
    memcpy(mac, inMac, 6);
}

espNowDirectSettingUpdate::~espNowDirectSettingUpdate()
{
    delete mac;
}

void ESPNowCTR::ConfigEspNowDirectNotif(uint8_t* mac, uint8_t notifByte, uint8_t dstSlaveID)
{
    esp_now_peer_info peerInfo;

    if (esp_now_get_peer(mac, &peerInfo) == ESP_ERR_ESPNOW_NOT_FOUND)
    {
        peerInfo = {};
        memcpy(peerInfo.peer_addr, mac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        esp_now_add_peer(&peerInfo);
    }

    fDirectNotif.push_back(new espNowDirectNotif(mac, notifByte, dstSlaveID));
}

void ESPNowCTR::ConfigEspNowDirectSettingUpdate(uint8_t* mac, uint8_t settingRef, uint8_t settingValueLen, uint8_t dstSlaveID)
{
    esp_now_peer_info peerInfo;

    DEBUG_PRINT_LN("CongigEspNowDirectSettingUpdate")
    DEBUG_PRINT_VALUE_BUF_LN("Mac", mac, 6)
    DEBUG_PRINT_VALUE("Ref", settingRef)
    DEBUG_PRINT_VALUE("dstSlave", dstSlaveID)

    if (esp_now_get_peer(mac, &peerInfo) == ESP_ERR_ESPNOW_NOT_FOUND)
    {
        peerInfo = {};
        memcpy(peerInfo.peer_addr, mac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        esp_now_add_peer(&peerInfo);
    }

    fDirectSettingUpdate.push_back(new espNowDirectSettingUpdate(mac, settingRef, dstSlaveID, settingValueLen));
}

void ESPNowCTR::SendDirectNotif(uint8_t notifByte)
{
    for (auto i = fDirectNotif.begin(); i != fDirectNotif.end(); i++)
    {
        if ((*i)->notifByte == notifByte)
        {
            size_t notifMsgSize = 7;

            uint8_t* notifBuffer = (uint8_t*)malloc(notifMsgSize * sizeof(uint8_t));

            notifBuffer[0] = Message::Frame::Start;
            notifBuffer[1] = 0;
            notifBuffer[2] = notifMsgSize;
            notifBuffer[3] = (*i)->dstSlaveID;
            notifBuffer[4] = Message::Type::Notif;
            notifBuffer[5] = notifByte;
            notifBuffer[6] = Message::Frame::End;
            
            esp_now_send((*i)->mac, notifBuffer, notifMsgSize);
        }
    }
}

void ESPNowCTR::SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen)
{
    DEBUG_PRINT_LN("Call to SendDirectSettingUpdate")
    for (auto i = fDirectSettingUpdate.begin(); i != fDirectSettingUpdate.end(); i++)
    {
        if ((*i)->settingRef == settingRef)
        {
            if (valueLen > (*i)->valueLen)
                valueLen = (*i)->valueLen;

            uint16_t msgSize = 8 + valueLen;

            uint8_t* msgBuffer = (uint8_t*)malloc(msgSize * sizeof(uint8_t));

            msgBuffer[0] = Message::Frame::Start;
            msgBuffer[1] = msgSize >> 8;
            msgBuffer[2] = msgSize;
            msgBuffer[3] = (*i)->dstSlaveID;
            msgBuffer[4] = Message::Type::SettingUpdate;
            msgBuffer[5] = (*i)->settingRef;
            msgBuffer[6] = valueLen;
            
            if (valueLen)
                memcpy(&(msgBuffer[7]), value, valueLen);

            msgBuffer[msgSize-1] = Message::Frame::End;
            
            DEBUG_PRINT_VALUE_BUF_LN("Message sent", msgBuffer, msgSize)

            esp_now_send((*i)->mac, msgBuffer, msgSize);
        }
    }
}
