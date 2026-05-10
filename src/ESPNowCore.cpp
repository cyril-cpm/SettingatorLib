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

		if (len == 7 &&
				data &&
				isBroadcastMac(des_addrArr))
		{
#if CONFIG_STR_SLAVE_ESPNOW
			if (*data == SLAVE_BROADCAST_PING)
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
					LOG("Slave not found");
					if (nbInitialisedSlave < CONFIG_STR_NB_SLAVE)
					{
						LOG("There is room dfor a slave");
						slave = slaveArray[nbInitialisedSlave];
						slave->get().SetEMac({data[1],
												data[2],
												data[3],
												data[4],
												data[5],
												data[6]});
						slave->get().Activate();
						nbInitialisedSlave++;
					}
					else
						LOG("To much slave initialised: %d", nbInitialisedSlave);
				}

				if (slave)
				{
					if (!slave->get().HasCTR(SlaveCTREnum::SLAVE_CTR_ESPNOW))
					{
						LOG("Creating CTR");
						ESPNowCTR& ctr = std::get<SLAVE_CTR_ESPNOW>
										(slave->get().GetCTRArray()[SLAVE_CTR_ESPNOW])
										.get();

						ctr.SetMac(src_addrArr);	
					}
					else
						slave->get().PlanifySendInitRequest();
				}
			}
#endif

#if CONFIG_STR_MASTER_ESPNOW
			if (*data == BRIDGE_BROADCAST_PING)
			{
				if (master.HasCTR(MASTER_CTR_ESPNOW))
				{
					ESPNowCTR& ctr = std::get<MASTER_CTR_ESPNOW>
									(master.GetCTRArray()[MASTER_CTR_ESPNOW])
									.get();

					if (ctr.GetMac() == src_addrArr)
					{
						LOG("BRIDGE_BROADCAST_PING from Master");
						ctr.PlanifySlaveIDTransmission();
					}
				}
			}
#endif
		}

		else
		{
			LOG("Not broadcasted Ping");
#if CONFIG_STR_MASTER_ESPNOW
			if (!master.HasCTR(MasterCTREnum::MASTER_CTR_ESPNOW))
			{
				ESPNowCTR& ctr = std::get<MASTER_CTR_ESPNOW>
								(master.GetCTRArray()[MASTER_CTR_ESPNOW])
								.get();
				ctr.SetMac(src_addrArr);
			}

			LOG("Message received");
#endif
			ESPNowCore::GetInstance().WriteToBuffer(data, len);

			if (info->rx_ctrl)
			{
				std::optional<std::reference_wrapper<ESPNowCTR>> ctr = GetESPNowCommunicatorByMac(src_addrArr);

				if (ctr)
				{
					ESP_LOGI("ESPNOWCORE", "registering linkinfo");
					ctr->get().SetLinkInfo(info->rx_ctrl->rssi,
											info->rx_ctrl->noise_floor,
											info->rx_ctrl->timestamp);
				}
			}

		}
	}
}

ESPNowCore::ESPNowCore()
{
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
