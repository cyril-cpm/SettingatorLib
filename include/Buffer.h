#pragma once

#include "Definitions.h"
#include "esp_log.h"
#include "esp_log_buffer.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include <array>
#include <cstdint>
#include <cstring>

class MessageBuffer
{
	public:

		MessageBuffer() = default;
		~MessageBuffer() = default;

		uint8_t& operator[] (uint16_t pos) {
			return fMessageBuffer[(pos >= CONFIG_STR_MESSAGE_BUFFER_SIZE ? 0 : pos)];
		}

		uint8_t*	data() { return fMessageBuffer.data(); }
		uint16_t	len() { return fLen; }

		void		SetLen(const uint16_t len) {
			fLen = len;
		}

	private:

		std::array<uint8_t, CONFIG_STR_MESSAGE_BUFFER_SIZE> fMessageBuffer;

		uint16_t	fLen = 0;
};

extern MessageBuffer messageBuffer;

class CircularBuffer
{
	public:

		bool			Write(const uint8_t* buf, const uint16_t len);

		const uint8_t&	operator[] (uint16_t pos) const {
			return fBuf[(fHead + pos) % CONFIG_STR_CIRCULAR_BUFFER_SIZE];
		}

		bool			CopyTo(MessageBuffer& destBuffer, uint16_t len) const {
			if (len > CONFIG_STR_MESSAGE_BUFFER_SIZE || len > CONFIG_STR_CIRCULAR_BUFFER_SIZE)
				return false;

			if (fHead + len > CONFIG_STR_CIRCULAR_BUFFER_SIZE)
			{
				uint16_t remaining = CONFIG_STR_CIRCULAR_BUFFER_SIZE - fHead;
				uint16_t overflow = len - remaining;

				if (std::memcpy(destBuffer.data(), fBuf.data() + fHead, remaining) &&
					std::memcpy(destBuffer.data() + remaining, fBuf.data(), overflow))
				{
					destBuffer.SetLen(len);
					return true;
				}
			}
			else
			{
				if (std::memcpy(destBuffer.data(), fBuf.data() + fHead, len))
				{
					destBuffer.SetLen(len);
					return true;
				}
			}
			return false;
		}

		uint16_t		GetContentLength() const {
			if (fTail >= fHead)
				return fTail - fHead;
			
			return (CONFIG_STR_CIRCULAR_BUFFER_SIZE - fHead) + fTail + 1;
		}

		void			CrunchGarbage() { fHead = fTail; }

		void			OffsetHead(uint16_t offset) {
			if ((fHead + offset) % CONFIG_STR_CIRCULAR_BUFFER_SIZE <= fTail)
				fHead = (fHead + offset) % CONFIG_STR_CIRCULAR_BUFFER_SIZE;
			else
				fHead = fTail;
		}

		void			OffsetTail(uint16_t offset) {
			uint16_t res = (fTail + offset) % CONFIG_STR_CIRCULAR_BUFFER_SIZE;

			if (res > GetRemainingLength())
				fTail = fHead;
			else
				fTail = res;

			ESP_LOGI("BUFFER", "offseting tail of %d", offset);
			ESP_LOGI("BUFFER", "new tail: %d", fTail);
		}

		uint16_t		GetRemainingLength() const {
			if (fHead > fTail)
				return fHead - fTail;

			return fHead + CONFIG_STR_CIRCULAR_BUFFER_SIZE - fTail;
		}

		uint16_t		GetContingousRemainingLength() const {
			if (fTail < fHead)
				return fHead - fTail;

			return CONFIG_STR_CIRCULAR_BUFFER_SIZE - fTail;
		}

		uint8_t*		GetHead() { return &fBuf[fHead]; }
		uint8_t*		GetTail() { return fBuf.data() + fTail; }
		uint8_t*		GetAfterTail() { return &fBuf[fTail+1]; }

		void			LogContent() const {
			ESP_LOG_BUFFER_HEX("BUFFER", &fBuf[fHead], fTail - fHead);
		}

	private:

		std::array<uint8_t, CONFIG_STR_CIRCULAR_BUFFER_SIZE>	fBuf;
		
		uint16_t	fHead = 0;
		uint16_t	fTail = 0;
};

