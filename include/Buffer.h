#pragma once

#include "Definitions.h"
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

	private:

		std::array<uint8_t, CONFIG_STR_CIRCULAR_BUFFER_SIZE>	fBuf;
		
		uint16_t	fHead = 0;
		uint16_t	fTail = 0;
};

