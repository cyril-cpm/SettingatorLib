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

            _receive(Message::CreateMessageAdoptBuffer(msg->data, msg->len));

            delete msg;
            espNowMsgList.pop();
        }
        espNowMsgListMutex.unlock();
    }
}

int ESPNowCTR::Write(Message& buf)
{
    esp_now_send(fMac, buf.GetBufPtr(), buf.GetLength());
    return 0;
}