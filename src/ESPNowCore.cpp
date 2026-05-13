#include "Core.h"
#include "Definitions.h"

#if STR_HAS_ESPNOW
#include "ESPNowCore.h"
#include "ESPNowCommunicator.h"
#include "STR.h"
#include "MiscDef.h"

#include <functional>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <nvs_flash.h>
#include "Slave.h"

#if CONFIG_STR_SLAVE_ESPNOW
#endif

#if CONFIG_STR_MASTER_ESPNOW
#include "Master.h"
#include "Settingator.h"

static Master& master = Master::GetInstance();
#endif

static const char* tag("ESPNowCore");

static bool isBroadcastMac(std::array<uint8_t, 6>& dstMac)
{
	return (dstMac == std::to_array<uint8_t>({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}));
}

void ESPNowCore::receiveCallback(const esp_now_recv_info* info, const uint8_t* data, int len)
{
	LOG("data received");
	 if (info)
	{
		std::array<uint8_t, 6> des_addrArr;
		if (info->des_addr)
			std::copy(info->des_addr, info->des_addr + 6, des_addrArr.begin());

		std::array<uint8_t, 6> src_addrArr;
		if (info->src_addr)
			std::copy(info->src_addr, info->src_addr + 6, src_addrArr.begin());

		LOG("len: %d", len);
		LOG("data: %d", *data);

		if (len && data)
		{
			switch (*data)
			{
#if CONFIG_STR_SLAVE_ESPNOW
				case SLAVE_BROADCAST_PING:
					if (len == 7)
					{
						ESP_LOGI(tag, "SLAVE_BROADCAST_PING");
						OptSlaveRef slave = GetSlaveForEMac({data[1],
															data[2],
															data[3],
															data[4],
															data[5],
															data[6]});

						if (initEspNowBroadcasted && !slave)
						{
							slave = CreateSlave({data[1],
												data[2],
												data[3],
												data[4],
												data[5],
												data[6]});
						}

						if (slave)
						{
							ESPNowCTR& ctr = slave->get().GetCTR<ESPNowCTR, SLAVE_CTR_ESPNOW>();

							if (!ctr)
							{
								ctr.SetMac(src_addrArr);
								ctr.PlanifyPingTimerCreation();
							}

							ctr.PlanifyBridgeToSlaveHandshake();

							if (slave->get().GetID())
								slave->get().PlanifySendInitRequest();
						}
					}
					break;

				case SLAVEID_TRANSMISSION_TO_BRIDGE:
					if (len == 8)
					{
						ESP_LOGI("ESPNOWCORE", "SLAVEID_TRANSMISSION %d", data[7]);
						OptSlaveRef slave = GetSlaveForEMac({data[1],
															data[2],
															data[3],
															data[4],
															data[5],
															data[6]});

						if (!slave)
						{
							slave = CreateSlave({data[1],
										data[2],
										data[3],
										data[4],
										data[5],
										data[6]}, data[7]);
						}
						if (slave)
						{
							ESPNowCTR& ctr = slave->get().GetCTR<ESPNowCTR, SLAVE_CTR_ESPNOW>();

							if (!ctr)
							{
								ctr.SetMac(src_addrArr);
								ctr.PlanifyPingTimerCreation();
							}

							if (slave->get().GetID())
								slave->get().PlanifySendInitRequest();
						}
					}
					break;
#endif

#if CONFIG_STR_MASTER_ESPNOW
				case BRIDGE_BROADCAST_PING:
					{
						ESPNowCTR& masterCtr = master.GetCTR<ESPNowCTR, MASTER_CTR_ESPNOW>();

						if (masterCtr.GetMac() == src_addrArr)
						{
							LOG("BRIDGE_BROADCAST_PING from Master");
							masterCtr.PlanifySlaveIDTransmission();
						}
					}
					break;

				case BRIDGE_TO_SLAVE_HANDSHAKE:
					{
						ESPNowCTR& ctr = master.GetCTR<ESPNowCTR, MASTER_CTR_ESPNOW>();

						if (!ctr)
							ctr.SetMac(src_addrArr);
						ESP_LOGI("ESPNowCore", "BRIDGE_TO_SLAVE_HANDSHAKE");
					}
					break;

				case LINK_PING:
					{
						ESPNowCTR& ctr = master.GetCTR<ESPNowCTR, MASTER_CTR_ESPNOW>();

						if (ctr)
							ctr.PlanifyPongSending();
					}
#endif

				case 0xFF:
					ESPNowCore::GetInstance().WriteToBuffer(data, len);

					if (info->rx_ctrl)
					{
						std::optional<std::reference_wrapper<ESPNowCTR>> ctr
									= GetESPNowCommunicatorByMac(src_addrArr);

						if (ctr)
						{
							ESP_LOGI("ESPNOWCORE", "registering linkinfo");
							ctr->get().SetLinkInfo(info->rx_ctrl->rssi,
													info->rx_ctrl->noise_floor,
													info->rx_ctrl->timestamp);
						}
					}
					break;
			}

		}
	}
}

void ESPNowCore::Init()
{
	LOG("InitImpl");

#if CONFIG_STR_SLAVE_ESPNOW
	initEspNowBroadcasted = false;
#endif

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

	ESP_ERROR_CHECK(esp_read_mac(fMac.data(), ESP_MAC_WIFI_STA));	// Récupération de l'adresse MAC Wi-Fi (STA)
}

void ESPNowCore::AddPeer(const std::array<uint8_t, 6>& peerMac)
{
	esp_now_peer_info peerInfo;

	if (esp_now_get_peer(peerMac.data(), &peerInfo) == ESP_ERR_ESPNOW_NOT_FOUND)
	{
		peerInfo = {};
		memcpy(peerInfo.peer_addr, peerMac.data(), 6);
		peerInfo.channel = 1;
		peerInfo.encrypt = false;

		esp_now_add_peer(&peerInfo);
	}
}

#endif
