#include "Buffer.h"
#include "Definitions.h"
#include <cstdint>
#include <cstring>
#include "MiscDef.h"
#include "esp_log_buffer.h"
#include "sdkconfig.h"

static const char* tag("BUFFER");

MessageBuffer messageBuffer;

bool CircularBuffer::Write(const uint8_t* buf, const uint16_t len)
{
	LOG("CircularBuffer::Write()");
	if (len >= CONFIG_STR_CIRCULAR_BUFFER_SIZE)
	{
		LOG("CircularBuffer::Write()");
		LOG("Buf len (%d) to long for CircularBuffer (%d)", len, CONFIG_STR_CIRCULAR_BUFFER_SIZE);
		ESP_LOG_BUFFER_HEX(tag, buf, len);
		return false;
	}

	if (fTail + len >= CONFIG_STR_CIRCULAR_BUFFER_SIZE)
	{
		uint16_t contingousRemaining = GetContingousRemainingLength();
		uint16_t overflow = len - contingousRemaining;

		if (overflow >= fHead)
		{
			LOG("CircularBuffer::Write()");
			LOG("Not enough space left in CircularBuffer (%d)", GetRemainingLength());
			ESP_LOG_BUFFER_HEX(tag, fBuf.data(), fBuf.size());
			LOG("fHead: %d\tfTail: %d", fHead, fTail);
			return false;
		}

		if (std::memcpy(fBuf.data() + fTail, buf, contingousRemaining) &&
			std::memcpy(fBuf.data(), buf + contingousRemaining, overflow))
		{
			fTail = overflow;
			return true;
		}
		else
		{
			LOG("Something failed with memcpy in CircularBuffer::Wrtie()");
		}
	}
	else
	{
		if (std::memcpy(fBuf.data() + fTail, buf, len))
		{
			fTail = fTail + len;
			return true;
		}
		else
		{
			LOG("Something failed with memcpy in CircularBuffer::Wrtie()");
		}
	}
	return false;
}

