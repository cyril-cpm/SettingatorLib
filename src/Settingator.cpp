#include "Settingator.h"

#include "Communicator.h"
#include "CommunicatorBridge.h"
#include "Setting.h"
#include "Message.h"
#include "MiscDef.h"
#include "ESPNowCommunicator.h"
#include "Slave.h"
#include <type_traits>
#include <variant>
//#include "CommandHandler.h"
#include <cstring>
#include <esp_log.h>
#include "esp_task_wdt.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "Led.h"


void Settingator::StartWiFi()
{

}

Settingator::Settingator(ICTR_t communicator)
{
	masterCTR = communicator;
}

Settingator::~Settingator()
{

}

void Settingator::SetNetLed(uint8_t r, uint8_t g, uint8_t b)
{
	if (fInfoLED)
		*fInfoLED = RGB(r, g, b);
}

void Settingator::ESPNowBroadcastPing()
{
	if (xPortInIsrContext())
		fShouldESPNowBroadcastPing = true;
	else
		ESPNowCore::GetInstance().BroadcastPing();
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

void Settingator::InitNetworkHID()
{
#if defined(STR_BRIDGE_HID)
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
	fInfoLED = new RGB(0, 255, 0);
	fInfoLEDStrip = new Strip(NET_HID_LED_PIN, fInfoLED, 1);
#endif
}

void Settingator::SetCommunicator(ICTR_t communicator)
{
	masterCTR = communicator;
}

void Settingator::Update()
{
	if (masterCTR.index())
	{
		if (ICTR_T_AVAILABLE(masterCTR))
		{
			Message* msg = ICTR_T_READ(masterCTR);

			if (!fSlaveID && msg && msg->GetType() == Message::Type::InitRequest)
				_createSlaveID(msg->GetSlaveID());

			if (fSlaveID && msg && *fSlaveID == msg->GetSlaveID())
			{
				auto msgType = msg->GetType();
	
				switch (msgType)
				{
				case Message::Type::InitRequest:
					_sendInitMessage();
					break;
	
				case Message::Type::SettingUpdate:
					_treatSettingUpdateMessage(*msg);
					break;
	
				case Message::Type::ConfigEspNowDirectNotif:
					_configEspNowDirectNotif(*msg);
					break;
	
				case Message::Type::ConfigEspNowDirectSettingUpdate:
					_configEspNowDirectSettingUpdate(*msg);
					break;
	
				case Message::Type::Notif:
					_treatNotifMessage(*msg);
					break;
	
				case Message::Type::RemoveDirectNotifConfig:
					_removeDirectNotifConfig(*msg);
					break;
	
				case Message::Type::RemoveDirectSettingUpdateConfig:
					_removeDirectSettingUpdateConfig(*msg);
					break;
	
				/*case Message::Type::Command:
					_treatCommandMessage(*msg);
					break;*/
				default:
					break;
				}

				ICTR_T_FLUSH(masterCTR);
			}
			else
			{
				if (fBridge)
					fBridge->Update();
				else
					ICTR_T_FLUSH(masterCTR);
			}
		}
	}

#if defined(ESP_PLATFORM)
	ESP_ERROR_CHECK(esp_task_wdt_reset());
	vTaskDelay(1);
#endif

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

	if (fInfoLEDStrip)
		fInfoLEDStrip->Show();
}

void Settingator::AddSetting(Setting& setting)
{
	fSettingVector.push_back(setting);
}

uint8_t Settingator::AddSetting(Setting::Type type, void* data_ptr, size_t data_size, const char* name, std::function<void()> callback)
{
	fSettingVector.push_back(Setting(type, data_ptr, data_size, name, callback, fInternalRefCount++));

	/*if (type != Setting::Type::Trigger)
	{
		void* buf = malloc(data_size * sizeof(uint8_t));
		size_t len = fPreferences->getBytes(name, buf, data_size);

		Setting* setting = GetSettingByRef(fInternalRefCount-1);

		if (setting && len)
		{
			setting->update((uint8_t*)buf, len);
		}
	}*/

	return(fInternalRefCount-1);
}

void Settingator::UpdateSetting(uint8_t ref, uint8_t* newValuePtr, size_t newValueSize)
{
	Setting* setting = GetSettingByRef(ref);

	if (setting)
	{
		setting->update(newValuePtr, newValueSize);
		SendUpdateMessage(setting);
	}
}

void Settingator::SendUpdateMessage(Setting* setting)
{
	if (setting)
	{
		std::visit([this, &setting](auto&& ctr) {
					
				using T = std::decay_t<decltype(ctr)>;

				if constexpr (!std::is_same_v<T, std::monostate>)
					ctr.Write(setting->buildUpdateMessage(fSlaveID));

			}, masterCTR);
	}
}

void Settingator::SendUpdateMessage(uint8_t ref)
{
	Setting* setting = GetSettingByRef(ref);

	if (setting)
		SendUpdateMessage(setting);
}

void Settingator::SendNotif(uint8_t notifByte)
{
	std::visit([this, notifByte](auto&& ctr) {

			using T = std::decay_t<decltype(ctr)>;

			if constexpr (!std::is_same_v<T, std::monostate>)
			{
				ctr.Write({
						Message::Frame::Start,
						0,
						7,
						*fSlaveID,
						Message::Type::Notif,
						notifByte,
						Message::Frame::End
					});
			}
		}, masterCTR);

}

void Settingator::SendDirectNotif(uint8_t notifByte)
{
	std::visit([notifByte](auto&& ctr) {

			using T = std::decay_t<decltype(ctr)>;

			if constexpr (!std::is_same_v<T, std::monostate>)
				ctr.SendDirectNotif(notifByte);

		}, masterCTR);
}

void Settingator::SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen)
{
	std::visit([settingRef, value, valueLen](auto&& ctr) {

			using T = std::decay_t<decltype(ctr)>;

			if constexpr (!std::is_same_v<T, std::monostate>)
				ctr.SendDirectSettingUpdate(settingRef, value, valueLen);
		}, masterCTR);
}

void Settingator::AddNotifCallback(void(*callback)(), uint8_t notifByte)
{
	fNotifCallback.push_back(new notifCallback(callback, notifByte));
}

Message Settingator::_buildSettingInitMessage()
{
	size_t initRequestSize = 7;


	for (Setting& setting : fSettingVector)
		initRequestSize += setting.getInitRequestSize();

	std::vector<uint8_t> requestBuffer(initRequestSize);

	requestBuffer[0] = Message::Frame::Start;
	requestBuffer[1] = initRequestSize >> 8;
	requestBuffer[2] = initRequestSize;
	requestBuffer[3] = *fSlaveID;
	requestBuffer[4] = Message::Type::SettingInit;
	requestBuffer[5] = fSettingVector.size();

	uint8_t* msgIndex = requestBuffer.data() + 6;

	for (Setting& setting : fSettingVector)
	{
		setting.getInitRequest(msgIndex);
		msgIndex += setting.getInitRequestSize();
	}

	requestBuffer[initRequestSize - 1] = Message::Frame::End;


	return Message(std::move(requestBuffer));
}

Setting* Settingator::GetSettingByRef(uint8_t ref)
{
	for (auto it = fSettingVector.begin(); it != fSettingVector.end(); it++)
	{
		if (it->getRef() == ref)
		{
			return &(*it);
		}
	}
	return nullptr;
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
		if (!fBridge)
			fBridge = CTRBridge::CreateInstance(masterCTR);

		if (fBridge)
		{
			fBridge->StartEspNowInitBroadcasted();
			SetNetLed(0, 0, 255);
		}
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
		if (fBridge)
		{
			fBridge->StopEspNowInitBroadcasted();
			SetNetLed(0, 255, 0);
		}
	}
}

void Settingator::begin()
{
	if (esp_task_wdt_status(nullptr) == ESP_ERR_NOT_FOUND)
		ESP_ERROR_CHECK(esp_task_wdt_add(nullptr));

	InitNetworkHID();
}

void Settingator::_createSlaveID(uint8_t slaveID)
{
	fSlaveID = new uint8_t;
	if (fSlaveID)
		*fSlaveID = slaveID;
}

void Settingator::_sendInitMessage()
{
	std::visit([this](auto&& ctr) {

			using T = std::decay_t<decltype(ctr)>;

			if constexpr (!std::is_same_v<T, std::monostate>)
				ctr.Write(_buildSettingInitMessage());

		}, masterCTR);
}

void Settingator::_treatSettingUpdateMessage(Message& msg)
{
	uint8_t* value;
	uint8_t ref;
	uint8_t valueLen;

	uint16_t nextSettingIndex = 5;

	do
	{
		nextSettingIndex = msg.ExtractSettingUpdate(ref, valueLen, &value, nextSettingIndex);

		Setting *setting = GetSettingByRef(ref);
	
		if (!setting)
		{
			//Serial.println("Setting Not found");
			//Serial.println(ref);
		}

		if (setting  && (valueLen == setting->getDataSize()))
		{
			//Serial.println("Attempt to memcpy");
			memcpy((void*)setting->getDataPtr(), value, valueLen);
			//Serial.println("Done");
		}
		else
		{
			//Serial.println("Value Len is 0")
		}

		if (setting)
			setting->callback();

	} while (msg.GetLength() > nextSettingIndex + 1);

}

void Settingator::_configEspNowDirectNotif(Message& msg)
{
	std::visit([&msg](auto&& ctr) {

			using T = std::decay_t<decltype(ctr)>;

			if constexpr (!std::is_same_v<T, std::monostate>)
			{
				uint8_t* buffer = msg.GetBufPtr();
				ctr.ConfigEspNowDirectNotif(&buffer[6], buffer[16], buffer[5]);
			}
		}, masterCTR);
}

void Settingator::_configEspNowDirectSettingUpdate(Message& msg)
{
	std::visit([&msg](auto&& ctr) {

			using T = std::decay_t<decltype(ctr)>;

			if constexpr (!std::is_same_v<T, std::monostate>)
			{
				uint8_t* buffer = msg.GetBufPtr();
				ctr.ConfigEspNowDirectSettingUpdate(&buffer[6], buffer[12], buffer[13], buffer[5]);
			}
		}, masterCTR);
}

void Settingator::_treatNotifMessage(Message& msg)
{
	auto buffer = msg.GetBufPtr();
	auto notifByte = buffer[5];

	for (notifCallback* cb : fNotifCallback)
	{
		if (cb->notifByte == notifByte)
			cb->callback();
	}
}

void Settingator::_removeDirectNotifConfig(Message& msg)
{
	std::visit([&msg](auto&& ctr) {

			using T = std::decay_t<decltype(ctr)>;

			if constexpr (!std::is_same_v<T, std::monostate>)
			{
				auto buffer = msg.GetBufPtr();
				ctr.RemoveDirectNotifConfig(buffer[5], buffer[6]);
			}
		}, masterCTR);
}

void Settingator::_removeDirectSettingUpdateConfig(Message& msg)
{
	std::visit([&msg](auto&& ctr) {

			using T = std::decay_t<decltype(ctr)>;

			if constexpr (!std::is_same_v<T, std::monostate>)
			{
				auto buffer = msg.GetBufPtr();
				ctr.RemoveDirectSettingUpdateConfig(buffer[5], buffer[6]);
			}
		}, masterCTR);
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
