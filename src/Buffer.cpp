#include "Buffer.h"
#include "Definitions.h"
#include <cstdint>
#include <cstring>
#include "MiscDef.h"

static const char* tag("BUFFER");

bool CircularBuffer::Write(const uint8_t* buf, const uint16_t len)
{
	if (fHead + len > CIRCULAR_BUFFER_SIZE)
	{
		uint16_t remaining = CIRCULAR_BUFFER_SIZE - fHead;
		uint16_t overflow = len - remaining;

		if (std::memcpy(fBuf.data() + fHead, buf, remaining) &&
			std::memcpy(fBuf.data(), buf + remaining, overflow))
		{
			fHead = overflow;
			return true;
		}
	}
	else
	{
		if (std::memcpy(fBuf.data() + fHead, buf, len))
		{
			fHead = fHead + len;
			return true;
		}
	}
	return false;
}

const uint8_t& CircularBuffer::operator[] (uint16_t pos) const
{
	return fBuf[pos % CIRCULAR_BUFFER_SIZE];
}
