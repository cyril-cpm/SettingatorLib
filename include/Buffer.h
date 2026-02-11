#pragma once

#include "Definitions.h"
#include <array>
#include <stdint.h>

class CircularBuffer
{
	public:

		bool			Write(const uint8_t* buf, const uint16_t len);
		const uint8_t&	operator[] (uint16_t pos) const;

	private:

		std::array<uint8_t, CIRCULAR_BUFFER_SIZE>	fBuf;
		
		uint16_t	fHead = 0;
		uint16_t	fTail = 0;
};

class MessageBuffer
{
	public:

		MessageBuffer() = default;
		~MessageBuffer() = default;

		uint8_t& operator[] (uint16_t pos) { return fMessageBuffer[(pos >= MESSAGE_BUFFER_SIZE ? 0 : pos)]; }

		uint8_t*	data() { return fMessageBuffer.data(); }
		uint16_t	len() { return fLen; }

	private:

		std::array<uint8_t, MESSAGE_BUFFER_SIZE> fMessageBuffer;

		uint16_t	fLen = 0;
};
extern MessageBuffer messageBuffer;
