#include "ESPNowCommunicator.h"
#include "MiscDef.h"
#include "Message.h"
#include <esp_now.h>
#include <WiFi.h>

espNowMsg::espNowMsg(const uint8_t* inData, int inLen) : len(inLen)
{
    data = (uint8_t*)malloc(sizeof(uint8_t) * len);
    memcpy(data, inData, len * sizeof(uint8_t));
}

ESPNowCTR*  ESPNowCTR::CreateInstance(uint8_t* mac)
{
    WiFi.mode(WIFI_STA);
    DEBUG_PRINT(WiFi.macAddress());

    return new ESPNowCTR(mac);
}

static void receiveCallback(const uint8_t* mac, const uint8_t* inData, int len)
{
    if (!esp_now_is_peer_exist(mac))
    {
        esp_now_peer_info peerInfo;

        memcpy(peerInfo.peer_addr, mac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        esp_now_add_peer(&peerInfo);
    }

    ESPNowCTR::espNowMsgList.push(espNowMsg(inData, len));
}

ESPNowCTR::ESPNowCTR(uint8_t* mac) : fMac(mac)
{
    esp_now_init();
    esp_now_register_recv_cb(receiveCallback);
}

ESPNowCTR::~ESPNowCTR()
{
    esp_now_deinit();
}

void ESPNowCTR::Update()
{
    while (ESPNowCTR::espNowMsgList.size())
    {
        auto msg = ESPNowCTR::espNowMsgList.front();

        _receive(new Message(msg.data, msg.len));

        ESPNowCTR::espNowMsgList.pop();
    }
}

int ESPNowCTR::Write(Message& buf)
{
    esp_now_send(nullptr, buf.GetBufPtr(), buf.GetLength());
    return 0;
}