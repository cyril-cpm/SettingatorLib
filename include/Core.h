#pragma once

#include <esp_log.h>
#include "Buffer.h"
#include "Message.h"
#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>

class ICore
{
	public:

		bool		FetchMessage() {
			
			uint16_t c = fBuf.GetContentLength();
			uint16_t i = 0;

			for (; i <= c && fBuf[i] != Message::Frame::Start; i++);

			fBuf.OffsetHead(i);

			if (i + 2 >= c)
				return false;

			uint16_t msgLength = (fBuf[1] << 8) + fBuf[2];

			if (msgLength > c) 
				return false;

			if (fBuf[msgLength] != Message::Frame::End)
			{
				for (; i <= c && fBuf[i] != Message::Frame::Start; i++);

				fBuf.OffsetHead(i);

				return false;
			}


			return true;
		}

		void		ThrowMessage() {
			ESP_LOGI("CORE", "ThrowMessage");

			uint16_t msgLength = (fBuf[1] << 8) + fBuf[2];

			fBuf.OffsetHead(msgLength);

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



