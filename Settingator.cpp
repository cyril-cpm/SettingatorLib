#include "Buffer.h"
#include "Definitions.h"

#include "Settingator.h"
#include "STR.h"

#include "Communicator.h"
#include "CommunicatorBridge.h"
#include "Setting.h"
#include "Message.h"
#include "MiscDef.h"
#include "ESPNowCommunicator.h"
#include "Slave.h"
#include <functional>
#include <optional>
#include <type_traits>
#include <variant>
//#include "CommandHandler.h"
#include <cstring>
#include <esp_log.h>
#include "esp_err.h"
#include "esp_task_wdt.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "Led.h"
#include "sdkconfig.h"

static const char* tag = "STR";

void Settingator::StartWiFi()
{

}

Settingator::Settingator()
#if defined (STR_BRIDGE_HID)
	:
fInfoLED(0, 255, 0),
fInfoLEDStrip(NET_HID_LED_PIN, &fInfoLED, 1)
#endif
{
}

Settingator::~Settingator()
{

}

#if (STR_BDRIDGE_HID)
void Settingator::SetNetLed(uint8_t r, uint8_t g, uint8_t b)
{
	fInfoLED = RGB(r, g, b);
}
#endif

void Settingator::ESPNowBroadcastPing()
{
#if STR_HAS_ESPNOW
	if (xPortInIsrContext())
		fShouldESPNowBroadcastPing = true;
	else
		ESPNowCore::GetInstance().BroadcastPing();
#endif
}

#if defined(STR_BRIDGE_HID)
#define DEBOUNCE_TIME_MS 250

static esp_timer_handle_t debounceTimerBroadcast;

static void debounceTimerBroadcastCallback(void*)
{
	gpio_intr_enable(BROADCAST_PIN);
}

static esp_timer_handle_t debounceTimerBridgeActivation;

static void debounceTimerBridgeActivationCallback(void*)
{
	gpio_intr_enable(BRIDGE_ACTIVATION_PIN);
}

static void IRAM_ATTR broadcastInterruptHandler(void* arg)
{
	gpio_intr_disable(BROADCAST_PIN);

	esp_timer_start_once(debounceTimerBroadcast, DEBOUNCE_TIME_MS * 1000);
	STR.ESPNowBroadcastPing();
}


static void IRAM_ATTR bridgeActivationInterruptHandler(void* arg)
{
	gpio_intr_disable(BRIDGE_ACTIVATION_PIN);

	esp_timer_start_once(debounceTimerBridgeActivation, DEBOUNCE_TIME_MS * 1000);
	if (initEspNowBroadcasted)
		STR.StopEspNowInitBroadcasted();
	else
		STR.StartEspNowInitBroadcasted();
}
#else
#pragma message("No Bridge HID")
#endif

#if defined(STR_BRIDGE_HID)
void Settingator::InitNetworkHID()
{
	// TIMER //
	esp_timer_create_args_t timer_args = {
		.callback = debounceTimerBroadcastCallback,
		.name = "debounce_timer_broadcast"
	};
	ESP_ERROR_CHECK(esp_timer_create(&timer_args, &debounceTimerBroadcast));

	esp_timer_create_args_t timer_args2 = {
		.callback = debounceTimerBridgeActivationCallback,
		.name = "debounce_timer_bridge_activation"
	};
	ESP_ERROR_CHECK(esp_timer_create(&timer_args2, &debounceTimerBridgeActivation));

	// GPIO CONFIG //
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = 1ULL << BROADCAST_PIN;
	ESP_ERROR_CHECK(gpio_config(&io_conf));

	gpio_config_t io_conf2 = {};
	io_conf2.intr_type = GPIO_INTR_DISABLE;
	io_conf2.pull_down_en = GPIO_PULLDOWN_ENABLE;
	io_conf2.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf2.mode = GPIO_MODE_INPUT;
	io_conf2.pin_bit_mask = 1ULL << BRIDGE_ACTIVATION_PIN;
	ESP_ERROR_CHECK(gpio_config(&io_conf2));

	ESP_ERROR_CHECK(gpio_install_isr_service(0));

	ESP_ERROR_CHECK(gpio_isr_handler_add(BROADCAST_PIN, broadcastInterruptHandler, (void*)BROADCAST_PIN));
	ESP_ERROR_CHECK(gpio_set_intr_type(BROADCAST_PIN, GPIO_INTR_POSEDGE));

	ESP_ERROR_CHECK(gpio_isr_handler_add(BRIDGE_ACTIVATION_PIN, bridgeActivationInterruptHandler, NULL));
	ESP_ERROR_CHECK(gpio_set_intr_type(BRIDGE_ACTIVATION_PIN, GPIO_INTR_POSEDGE));
	// fInfoLED =  RGB(0, 255, 0);
	// fInfoLEDStrip = Strip(NET_HID_LED_PIN, &fInfoLED, 1);
}
#endif

void Settingator::Update()
{
	for (ICore& core : coreArray)
	{
		if (core.FetchMessage())
		{
			if (core.GetSrcSlaveID() == 0)
			{
				if (!fSlaveID && core.GetMessageType() == Message::Type::InitRequest)
					fSlaveID = core.GetDstSlaveID();

				if (fSlaveID == core.GetDstSlaveID())
				{
					switch (core.GetMessageType())
					{
						case Message::Type::InitRequest:
							LOG("InitRequest");
							_sendInitMessage();
							break;

						case Message::Type::SettingUpdate:
							_treatSettingUpdateMessage(core);
							break;

						// Direct Msg/Notif and notif addressed to Slave

						default:
							LOG("Unknown Message");
							break;
					}
					core.ThrowMessage();
				}
				else
				{
					core.ThrowMessage();
					//bridge handling
				}
			}
		}

	}

	ESP_ERROR_CHECK(esp_task_wdt_reset());
	vTaskDelay(1);

	if (fShouldStartEspNowInitBroadcasted)
	{
		StartEspNowInitBroadcasted();
		fShouldStartEspNowInitBroadcasted = false;
	}

	if (fShouldStopEspNowInitBroadcasted)
	{
		StopEspNowInitBroadcasted();
		fShouldStopEspNowInitBroadcasted = false;
	}

	if (fShouldESPNowBroadcastPing)
	{
		ESPNowBroadcastPing();
		fShouldESPNowBroadcastPing = false;
	}

#if defined(STR_BRIDGE_HID)
	fInfoLEDStrip.Show();
#endif
}

// void Settingator::_Update()
// {
// 	if (masterCTR.index())
// 	{
// 		if (ICTR_T_AVAILABLE(masterCTR))
// 		{
// 			Message* msg = ICTR_T_READ(masterCTR);
//
// 			if (!fSlaveID && msg && msg->GetType() == Message::Type::InitRequest)
// 				_createSlaveID(msg->GetSlaveID());
//
// 			if (msg && fSlaveID == msg->GetSlaveID())
// 			{
// 				auto msgType = msg->GetType();
//
// 				switch (msgType)
// 				{
// 				case Message::Type::InitRequest:
// 					_sendInitMessage();
// 					break;
//
// 				case Message::Type::SettingUpdate:
// 					_treatSettingUpdateMessage(*msg);
// 					break;
//
// 				case Message::Type::ConfigEspNowDirectNotif:
// 					_configEspNowDirectNotif(*msg);
// 					break;
//
// 				case Message::Type::ConfigEspNowDirectSettingUpdate:
// 					_configEspNowDirectSettingUpdate(*msg);
// 					break;
//
// 				case Message::Type::Notif:
// 					_treatNotifMessage(*msg);
// 					break;
//
// 				case Message::Type::RemoveDirectNotifConfig:
// 					_removeDirectNotifConfig(*msg);
// 					break;
//
// 				case Message::Type::RemoveDirectSettingUpdateConfig:
// 					_removeDirectSettingUpdateConfig(*msg);
// 					break;
//
// 				/*case Message::Type::Command:
// 					_treatCommandMessage(*msg);
// 					break;*/
// 				default:
// 					break;
// 				}
//
// 				ICTR_T_FLUSH(masterCTR);
// 			}
// 			else
// 			{
// 				if (fBridge)
// 					fBridge->Update();
// 				else
// 					ICTR_T_FLUSH(masterCTR);
// 			}
// 		}
// 	}
//
// #if defined(ESP_PLATFORM)
// 	ESP_ERROR_CHECK(esp_task_wdt_reset());
// 	vTaskDelay(1);
// #endif
//
// 	if (fShouldStartEspNowInitBroadcasted)
// 	{
// 		StartEspNowInitBroadcasted();
// 		fShouldStartEspNowInitBroadcasted = false;
// 	}
//
// 	if (fShouldStopEspNowInitBroadcasted)
// 	{
// 		StopEspNowInitBroadcasted();
// 		fShouldStopEspNowInitBroadcasted = false;
// 	}
//
// 	if (fShouldESPNowBroadcastPing)
// 	{
// 		ESPNowBroadcastPing();
// 		fShouldESPNowBroadcastPing = false;
// 	}
//
// #if defined(STR_BRIDGE_HID)
// 	fInfoLEDStrip.Show();
// #endif
// }

uint8_t Settingator::AddSetting(Setting::Type type, void* data_ptr, size_t data_size, const char* name, std::function<void()> callback)
{
	if (fInternalRefCount >= CONFIG_STR_NB_SETTINGS)
	{
		LOG("To much settings");
		return CONFIG_STR_NB_SETTINGS;
	}

	fSettingArray[fInternalRefCount] = Setting(type, data_ptr,
												data_size, name,
												callback, fInternalRefCount);

	return (fInternalRefCount++);
}

void Settingator::UpdateSetting(uint8_t ref, uint8_t* newValuePtr, size_t newValueSize)
{
	std::optional<Setting> setting = GetSettingByRef(ref);

	if (setting.has_value())
	{
		setting->update(newValuePtr, newValueSize);
		SendUpdateMessage(*setting);
	}
}

void Settingator::SendUpdateMessage(Setting& setting)
{
    uint16_t messageLength = 9 + setting.getDataSize();

	if (messageLength > CONFIG_STR_MESSAGE_BUFFER_SIZE)
	{
		LOG("updateMessageLength > CONFIG_STR_MESSAGE_BUFFER_SIZE");
		return;
	}

    messageBuffer[0] = Message::Frame::Start;

    messageBuffer[1] = messageLength >> 8;
    messageBuffer[2] = messageLength;

    messageBuffer[3] = fSlaveID;
	messageBuffer[4] = 0;

    messageBuffer[5] = Message::Type::SettingUpdate;

    messageBuffer[6] = setting.getRef();
    messageBuffer[7] = setting.getDataSize();
    memcpy(messageBuffer.data() + 8, setting.getDataPtr(), setting.getDataSize());
    messageBuffer[messageLength - 1] = Message::Frame::End;

	master.Write();
}

void Settingator::SendUpdateMessage(uint8_t ref)
{
	std::optional<std::reference_wrapper<Setting>> setting = GetSettingByRef(ref);

	if (setting)
		SendUpdateMessage(setting->get());
}

void Settingator::SendNotif(uint8_t notifByte)
{
	master.Write({
			Message::Frame::Start,
			0,
			8,
			fSlaveID,
			0,
			Message::Type::Notif,
			notifByte,
			Message::Frame::End
		});
}

void Settingator::SendDirectNotif(uint8_t notifByte)
{
	// std::visit([notifByte](auto&& ctr) {
	//
	// 		using T = std::decay_t<decltype(ctr)>;
	//
	// 		if constexpr (!std::is_same_v<T, std::monostate>)
	// 			ctr.SendDirectNotif(notifByte);
	//
	// 	}, masterCTR);
}

void Settingator::SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen)
{
	// std::visit([settingRef, value, valueLen](auto&& ctr) {
	//
	// 		using T = std::decay_t<decltype(ctr)>;
	//
	// 		if constexpr (!std::is_same_v<T, std::monostate>)
	// 			ctr.SendDirectSettingUpdate(settingRef, value, valueLen);
	// 	}, masterCTR);
}

void Settingator::AddNotifCallback(void(*callback)(), uint8_t notifByte)
{
	// fNotifCallback.push_back(notifCallback(callback, notifByte));
}

std::optional<std::reference_wrapper<Setting>> Settingator::GetSettingByRef(uint8_t ref)
{
	if (ref >= CONFIG_STR_NB_SETTINGS)
		return std::nullopt;

	auto& setting = fSettingArray[ref];

	if (setting)
		return *setting;

	return std::nullopt;
}

void Settingator::SavePreferences()
{
}

void Settingator::StartEspNowInitBroadcasted()
{
	if (xPortInIsrContext())
	{
		fShouldStartEspNowInitBroadcasted = true;
	}
	else
	{
#if CONFIG_STR_HAS_BRIDGE
		CTRBridge::GetInstance().StartEspNowInitBroadcasted();
#if defined(STR_BRIDGE_HID)
			SetNetLed(0, 0, 255);
#endif
#endif
	}
}

void Settingator::StopEspNowInitBroadcasted()
{
	if (xPortInIsrContext())
	{
		fShouldStopEspNowInitBroadcasted = true;
	}
	else
	{
#if CONFIG_STR_HAS_BRIDGE
		CTRBridge::GetInstance().StopEspNowInitBroadcasted();
#if defined(STR_BRIDGE_HID)
			SetNetLed(0, 255, 0);
#endif
#endif
	}
}

void Settingator::begin()
{
	if (esp_task_wdt_status(nullptr) == ESP_ERR_NOT_FOUND)
		ESP_ERROR_CHECK(esp_task_wdt_add(nullptr));

#if defined(STR_BRIDGE_HID)
	InitNetworkHID();
#endif
}

void Settingator::_createSlaveID(uint8_t slaveID)
{
	fSlaveID = slaveID;
}

void Settingator::_sendInitMessage()
{
	uint16_t initRequestSize = 8;


	for (const auto& setting : fSettingArray)
	{
		if (setting)
			initRequestSize += setting->getInitRequestSize();
		else
			break;
	}

	if (initRequestSize > CONFIG_STR_MESSAGE_BUFFER_SIZE)
	{
		LOG("initRequest Size > CONFIG_STR_MESSAGE_BUFFER_SIZE");
		return;
	}

	messageBuffer[0] = Message::Frame::Start;
	messageBuffer[1] = initRequestSize >> 8;
	messageBuffer[2] = initRequestSize;
	messageBuffer[3] = fSlaveID;
	messageBuffer[4] = 0;
	messageBuffer[5] = Message::Type::SettingInit;
	messageBuffer[6] = fInternalRefCount;

	uint16_t msgIndex = 7;

	for (const auto& setting : fSettingArray)
	{
		if (setting)
		{
			setting->getInitRequest(msgIndex);
			msgIndex += setting->getInitRequestSize();
		}
		else
			break;
	}

	messageBuffer[initRequestSize - 1] = Message::Frame::End;

	messageBuffer.SetLen(initRequestSize);

	master.Write();
}

void Settingator::_treatSettingUpdateMessage(const ICore& core)
{
	const uint8_t* value;
	uint8_t ref;
	uint8_t valueLen;

	uint16_t settingIndex = 5;

	do
	{
		// nextSettingIndex = msg.ExtractSettingUpdate(ref, valueLen, &value, nextSettingIndex);

		ref = core[settingIndex];
		valueLen = core[settingIndex + 1];
		value = &core[settingIndex + 2];
		settingIndex = settingIndex + 2 + valueLen;
	
		const auto& settingOptRef = GetSettingByRef(ref);
	
		if (!settingOptRef)
		{
			//Serial.println("Setting Not found");
			//Serial.println(ref);
		}

		if (settingOptRef)
		{
			Setting& setting = settingOptRef->get();

			if (valueLen == setting.getDataSize())
			{
				//Serial.println("Attempt to memcpy");
				memcpy((void*)setting.getDataPtr(), value, valueLen);
				//Serial.println("Done");
			}
			else
			{
				//Serial.println("Value Len is 0")
			}

			setting.callback();
		}
	} while (core.GetMessageSize() > settingIndex + 1);

}

void Settingator::_treatNotifMessage(const ICore& core)
{
	uint8_t notifByte = core[5];

	for (notifCallback& cb : fNotifCallback)
	{
		if (cb.notifByte == notifByte)
			cb.callback();
	}
}

void Settingator::_configEspNowDirectNotif(Message& msg)
{
	// std::visit([&msg](auto&& ctr) {
	//
	// 		using T = std::decay_t<decltype(ctr)>;
	//
	// 		if constexpr (!std::is_same_v<T, std::monostate>)
	// 		{
	// 			uint8_t* buffer = msg.GetBufPtr();
	// 			ctr.ConfigEspNowDirectNotif(&buffer[6], msg[16], msg[5]);
	// 		}
	// 	}, masterCTR);
}

void Settingator::_configEspNowDirectSettingUpdate(Message& msg)
{
	// std::visit([&msg](auto&& ctr) {
	//
	// 		using T = std::decay_t<decltype(ctr)>;
	//
	// 		if constexpr (!std::is_same_v<T, std::monostate>)
	// 		{
	// 			uint8_t* buffer = msg.GetBufPtr();
	// 			ctr.ConfigEspNowDirectSettingUpdate(&buffer[6], buffer[12], buffer[13], buffer[5]);
	// 		}
	// 	}, masterCTR);
}


void Settingator::_removeDirectNotifConfig(Message& msg)
{
	// std::visit([&msg](auto&& ctr) {
	//
	// 		using T = std::decay_t<decltype(ctr)>;
	//
	// 		if constexpr (!std::is_same_v<T, std::monostate>)
	// 		{
	// 			auto buffer = msg.GetBufPtr();
	// 			ctr.RemoveDirectNotifConfig(buffer[5], buffer[6]);
	// 		}
	// 	}, masterCTR);
}

void Settingator::_removeDirectSettingUpdateConfig(Message& msg)
{
	// std::visit([&msg](auto&& ctr) {
	//
	// 		using T = std::decay_t<decltype(ctr)>;
	//
	// 		if constexpr (!std::is_same_v<T, std::monostate>)
	// 		{
	// 			auto buffer = msg.GetBufPtr();
	// 			ctr.RemoveDirectSettingUpdateConfig(buffer[5], buffer[6]);
	// 		}
	// 	}, masterCTR);
}

/*void Settingator::_treatCommandMessage(Message msg)
{
	if (!msg || !masterCTR)
		return;

	auto buffer = msg->GetBufPtr();

	char* cmdBuffer = (char*)&(buffer[4]);

	if (fCommandHandler)
		fCommandHandler->TreatCommand(cmdBuffer);
}*/

setting_ref Settingator::settingRefCount()
{
	return fInternalRefCount++;
}

//Settingator STR(nullptr);
