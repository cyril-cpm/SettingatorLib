#include "Buffer.h"
#include "Definitions.h"
#include <cstdint>
#include <cstring>
#include "MiscDef.h"
#include "sdkconfig.h"

static const char* tag("BUFFER");

MessageBuffer messageBuffer;

bool CircularBuffer::Write(const uint8_t* buf, const uint16_t len)
{
	if (len > CONFIG_STR_CIRCULAR_BUFFER_SIZE)
		return false;

	if (fHead + len > CONFIG_STR_CIRCULAR_BUFFER_SIZE)
	{
		uint16_t remaining = CONFIG_STR_CIRCULAR_BUFFER_SIZE - fHead;
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

