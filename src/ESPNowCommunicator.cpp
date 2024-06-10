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

ESPNowCTR*  ESPNowCTR::CreateInstanceDiscoverableWithSSID(const char* deviceName)
{
    WiFi.mode(WIFI_AP);
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
    esp_now_peer_info peerInfo = {};

    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_now_add_peer(&peerInfo);

    espNowMsgList.push(espNowMsg(inData, len));

    WiFi.softAPdisconnect();
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
    while (espNowMsgList.size())
    {
        auto msg = espNowMsgList.front();

        _receive(new Message(msg.data, msg.len));

        espNowMsgList.pop();
    }
}

int ESPNowCTR::Write(Message& buf)
{
    esp_now_send(fMac, buf.GetBufPtr(), buf.GetLength());
    return 0;
}

std::queue<espNowMsg> espNowMsgList;