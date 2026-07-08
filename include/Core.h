#pragma once

#include <cstdlib>
#include <esp_log.h>
#include "Buffer.h"
#include "Message.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>

#define SLAVE_BROADCAST_PING 0x01
#define BRIDGE_BROADCAST_PING 0x02
#define LINK_PING 0x03
#define LINK_PONG 0x04
#define SLAVEID_TRANSMISSION_TO_BRIDGE 0x05
#define BRIDGE_TO_SLAVE_HANDSHAKE 0x06

enum CoreEnum {
#if STR_HAS_ESPNOW
	CORE_ESPNOW,
#endif

#if STR_HAS_UART0
	CORE_UART0,
#endif

#if STR_HAS_UART1
	CORE_UART1,
#endif

#if STR_HAS_UART2
	CORE_UART2,
#endif

#if STR_HAS_LORA
	CORE_LORA,
#endif

	CORE_MAX
};

class ICore
{
	public:

		bool		FetchMessage() {
			return fBuf.Fetch(Message::Frame::Start, Message::Frame::End, "CORE");
		}

		void		ThrowMessage() {
			ESP_LOGD("CORE", "ThrowMessage");

			uint16_t msgLength = (fBuf[1] << 8) + fBuf[2];

			ESP_LOGD("CORE", "Offseting Head from %d of %d", fBuf.GetHeadPos(), msgLength);
			fBuf.OffsetHead(msgLength);
			ESP_LOGD("CORE", "res: %d", fBuf.GetHeadPos());

		}

		uint8_t		GetSrcSlaveID() const { return fBuf[3]; }
		uint8_t		GetDstSlaveID() const { return fBuf[4]; }
		uint8_t		GetMessageType() const { return fBuf[5]; }
		uint16_t	GetMessageSize() const { return (fBuf[1] << 8) + fBuf[2]; };

		void		WriteToBuffer(const uint8_t* buf, const uint16_t len) {
			fBuf.Write(buf, len);
		}

		void		CopyMessageToGlobalBuffer() {
			fBuf.CopyTo(messageBuffer, GetMessageSize());
		}

		const uint8_t&	operator[](uint16_t index) const { return fBuf[index]; }
	
		void		SemGive() { 
			BaseType_t h = pdFALSE;
			xSemaphoreGiveFromISR(fSem, &h);
			if (h)
				portYIELD_FROM_ISR();
		}
		void		SemTake() { xSemaphoreTake(fSem, portMAX_DELAY); }
	protected:

		ICore() {
			fSem = xSemaphoreCreateBinaryStatic(&fSemBuf);
			xSemaphoreGive(fSem);
		}

		CircularBuffer fBuf;
		StaticSemaphore_t	fSemBuf;
		SemaphoreHandle_t	fSem;

};

inline TaskHandle_t mainTaskHandle = nullptr;

extern std::array<std::reference_wrapper<ICore>, CORE_MAX> coreArray;

void		InitCores();
