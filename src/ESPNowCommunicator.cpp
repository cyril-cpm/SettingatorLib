#include "ESPNowCommunicator.h"
#include "MiscDef.h"
#include "Message.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_mac.h>
#include <mutex>

#if defined(ARDUINO)

#elif defined(ESP_PLATFORM)
#include <cstring>
#include <esp_log.h>
#endif

ESPNowCore* espNowCore = nullptr;

std::mutex espNowMsgListMutex;

std::vector<std::pair<uint8_t*, std::queue<espNowMsg*>>> espNowMsgList;

//std::queue<espNowMsg*> espNowMsgList;

static bool compareMac(const uint8_t* mac1, const uint8_t* mac2) {
    return memcmp(mac1, mac2, 6) == 0;
}

static std::queue<espNowMsg*>* findQueueForMac(const uint8_t* macToFind) {
    for (auto& pair : espNowMsgList) {
        if (compareMac(pair.first, macToFind)) {
            return &pair.second;
        }
    }
    return nullptr; // Si non trouvé
}

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


static bool isBroadcastMac(uint8_t* dstMac)
{
    bool isBroadcastMac = true;

    if (dstMac)
    {
        for (auto i = 0; i < 6; i++)
        {
            if (dstMac[i] != 0xFF)
                isBroadcastMac = false;
        }
    }
    else
        isBroadcastMac = false;

    return isBroadcastMac;
}

#if defined(ARDUINO)
static void receiveCallback(const uint8_t* mac, const uint8_t* data, int len)
{
    if (mac)
    {
        if (initEspNowBroadcasted && len == 1 && data && *data == 0x42)
        {
            ICTR* newCTR = ESPNowCTR::CreateInstanceWithMac(mac);

            newSlavesCTR.push(newCTR);
        }
        else
        {
            if (!masterCTR) //A changer
                masterCTR = ESPNowCTR::CreateInstanceWithMac(mac);

            espNowMsgListMutex.lock();

            auto list = findQueueForMac(mac);

            if (list)
                list->push(new espNowMsg(data, len));

            espNowMsgListMutex.unlock();
        }
    }
}

#elif defined(ESP_PLATFORM)
static void receiveCallback(const esp_now_recv_info* info, const uint8_t* data, int len)
{
    if (info)
    {
        if (initEspNowBroadcasted && len == 1 && data && *data == 0x42 && isBroadcastMac(info->des_addr))
        {
            ICTR* newCTR = ESPNowCTR::CreateInstanceWithMac(info->src_addr);

            newSlavesCTR.push(newCTR);
        }
        else
        {
            if (!masterCTR)
                masterCTR = ESPNowCTR::CreateInstanceWithMac(info->src_addr);

            espNowMsgListMutex.lock();
            
            auto list = findQueueForMac(info->src_addr);

            if (list)
                list->push(new espNowMsg(data, len));

            espNowMsgListMutex.unlock();
        }
    }
}

#endif

ESPNowCore* ESPNowCore::CreateInstance()
{
    if (!espNowCore)
        espNowCore = new ESPNowCore();

    return espNowCore;
}

ESPNowCore::ESPNowCore()
{
    //NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());  // Efface la mémoire si nécessaire
        ret = nvs_flash_init();  // Réinitialise NVS
    }

    //WIFI
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        ESP_ERROR_CHECK(err);

    wifi_init_config_t wifiCfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifiCfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));

    //NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(receiveCallback));
    
    uint8_t mac[6];  // Tableau pour stocker l'adresse MAC
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));  // Récupération de l'adresse MAC Wi-Fi (STA)

    ESP_LOGI("MAC", "Adresse MAC (STA) : %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);    

}

int ESPNowCore::Write(Message& buf, uint8_t* dstMac)
{
    //ESP_LOGI("ESPNowCTR", "Esp now write");

    //DEBUG_PRINT_LN(buf.GetLength())
    //if (buf.GetLength() > 250)
    //   DEBUG_PRINT_LN("Alert ! message to big for ESPNow !")

    //ESP_LOG_BUFFER_HEX("ESPNowCore", dstMac, 6);
    esp_now_send(dstMac, buf.GetBufPtr(), buf.GetLength());
    return 0;
}

void ESPNowCore::AddPeer(uint8_t* peerMac)
{
    esp_now_peer_info peerInfo;

    if (esp_now_get_peer(peerMac, &peerInfo) == ESP_ERR_ESPNOW_NOT_FOUND)
    {
        peerInfo = {};
        memcpy(peerInfo.peer_addr, peerMac, 6);
        peerInfo.channel = 1;
        peerInfo.encrypt = false;

        esp_now_add_peer(&peerInfo);
    }
}

void ESPNowCore::BroadcastPing()
{
    uint8_t dstMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t data = 0x42;

    AddPeer(dstMac);

    ESP_ERROR_CHECK(esp_now_send(dstMac, &data, sizeof(data)));
}

ESPNowCTR* ESPNowCTR::CreateInstanceWithMac(const uint8_t* mac)
{
   
    return new ESPNowCTR(mac);
}

ESPNowCTR::ESPNowCTR(const uint8_t* peerMac)
{

    fCore = ESPNowCore::CreateInstance();

    if (peerMac != nullptr)
    {
        ESP_LOGI("ESPNowCTR", "mac not null");
        fMac = (uint8_t*)malloc(6 * sizeof(uint8_t));
        memcpy(fMac, peerMac, 6 * sizeof(uint8_t));
        
        fCore->AddPeer(fMac);

        std::queue<espNowMsg*> msgList;

        espNowMsgList.emplace_back(fMac, msgList); 
    }

    ESP_LOGI("ESPNowCTR", "end init");
}

ESPNowCTR::~ESPNowCTR()
{
    esp_now_deinit();
}

void ESPNowCTR::_bufferizeMessage(espNowMsg* msg)
{
    if (fMessageBuffer == nullptr)
    {
        fMessageBuffer = (uint8_t*)malloc(msg->len * sizeof(uint8_t));
        fMessageBufferSize = msg->len;
        memcpy(fMessageBuffer, msg->data, msg->len);
    }
    else
    {
        uint8_t* tmpBuf = fMessageBuffer;

        fMessageBuffer = (uint8_t*)malloc((fMessageBufferSize + msg->len) * sizeof(uint8_t));
        memcpy(fMessageBuffer, tmpBuf, fMessageBufferSize);
        memcpy(fMessageBuffer + fMessageBufferSize, msg->data, msg->len);
        fMessageBufferSize += msg->len;
        delete tmpBuf;
    }

    uint16_t msgSize = 0;
            
    if (fMessageBufferSize >= 3)
        msgSize = (fMessageBuffer[1] << 8) + fMessageBuffer[2];

    if (msgSize <= fMessageBufferSize && fMessageBuffer[msgSize-1] == Message::Frame::End)
    {
        _receive(new Message(fMessageBuffer, fMessageBufferSize));
        fMessageBufferSize = 0;
        delete fMessageBuffer;
    }
}

void ESPNowCTR::Update()
{
    if (espNowMsgListMutex.try_lock())
    {
        auto list = findQueueForMac(fMac);


        while (list && list->size())
        {
            auto msg = list->front();

            uint16_t msgSize = 0;
            
            if (msg->len >= 3)
                msgSize = (msg->data[1] << 8) + msg->data[2];

            if (msgSize > msg->len || msg->data[msgSize-1] != Message::Frame::End)
                _bufferizeMessage(msg);
            else
                _receive(new Message(msg->data, msg->len));

            delete msg;
            list->pop();
        }
        espNowMsgListMutex.unlock();
    }
}

int ESPNowCTR::Write(Message& buf)
{
    DEBUG_PRINT_LN(buf.GetLength())
    auto bufLength = buf.GetLength();

    for (int i = 0; i < bufLength; i += 250)
        esp_now_send(fMac, buf.GetBufPtr() + i, (bufLength - i) > 250 ? 250 : (bufLength - i));

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
    fCore->AddPeer(mac);

    fDirectNotif.push_back(new espNowDirectNotif(mac, notifByte, dstSlaveID));
}

void ESPNowCTR::ConfigEspNowDirectSettingUpdate(uint8_t* mac, uint8_t settingRef, uint8_t settingValueLen, uint8_t dstSlaveID)
{
    fCore->AddPeer(mac);

    fDirectSettingUpdate.push_back(new espNowDirectSettingUpdate(mac, settingRef, dstSlaveID, settingValueLen));
}

void ESPNowCTR::RemoveDirectNotifConfig(uint8_t dstSlaveID, uint8_t notifByte)
{
    for (auto i = fDirectNotif.begin(); i != fDirectNotif.end(); i++)
    {
        if ((*i)->dstSlaveID == dstSlaveID && (*i)->notifByte == notifByte)
        {
            fDirectNotif.erase(i);
            break;
        }
    }
}

void ESPNowCTR::RemoveDirectSettingUpdateConfig(uint8_t dstSlaveID, uint8_t settingRef)
{
    for (auto i = fDirectSettingUpdate.begin(); i != fDirectSettingUpdate.end(); i++)
    {
        if ((*i)->dstSlaveID == dstSlaveID && (*i)->settingRef == settingRef)
        {
            fDirectSettingUpdate.erase(i);
            break;
        }
    }
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
            
            Message notifMessage(&notifBuffer, 7);
            fCore->Write(notifMessage, (*i)->mac);
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
            Message msg(&msgBuffer, msgSize);
            fCore->Write(msg, (*i)->mac);
        }
    }
}
