#pragma once

#include <cstdlib>
#include <esp_log.h>
#include "Buffer.h"
#include "Message.h"
#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>

#define SLAVE_BROADCAST_PING 0x01
#define BRIDGE_BROADCAST_PING 0x02
#define LINK_PING 0x03
#define LINK_PONG 0x04

class ICore
{
	public:

		bool		FetchMessage() {
			
			uint16_t c = fBuf.GetContentLength();

			if (!c)
				return false;

			uint16_t i = 0;

			for (; i <= c && fBuf[i] != Message::Frame::Start; i++);

			fBuf.OffsetHead(i);

			if (i + 2 >= c)
			{
				// ESP_LOGI("CORE", "Not enough content %d", c);
				return false;
			}

			uint16_t msgLength = (fBuf[1] << 8) + fBuf[2];

			if (msgLength > c) 
			{
				ESP_LOGI("CORE", "msgLength %d > content %d", msgLength, c);
				ESP_LOGI("CORE", "head: %d, tail: %d", fBuf.GetHeadPos(), fBuf.GetTailPos());
				fBuf.LogWholeBuffer();
				return false;
			}

			if (fBuf[msgLength] != Message::Frame::End)
			{
				ESP_LOGI("CORE", "End Frame not found at fBuf[msgLength] %d", fBuf[msgLength]);

				ESP_LOGI("CORE", "head: %d\ttail: %d", fBuf.GetHeadPos(), fBuf.GetTailPos());
				fBuf.LogContent();
				fBuf.LogWholeBuffer();


				for (; i <= c && fBuf[i] != Message::Frame::Start; i++);

				fBuf.OffsetHead(i);

				ESP_LOGI("CORE", "head offseted of %d, result is %d", i, fBuf.GetHeadPos());
				abort();
				return false;
			}

			return true;
		}

		void		ThrowMessage() {
			ESP_LOGI("CORE", "ThrowMessage");

			uint16_t msgLength = (fBuf[1] << 8) + fBuf[2];

			ESP_LOGI("CORE", "Offseting Head from %d of %d", fBuf.GetHeadPos(), msgLength);
			fBuf.OffsetHead(msgLength);
			ESP_LOGI("CORE", "res: %d", fBuf.GetHeadPos());

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
	
	protected:

		CircularBuffer fBuf;

};



