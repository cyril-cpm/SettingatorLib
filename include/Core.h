#pragma once

#include "Buffer.h"
#include <cstdint>

class ICore
{
	public:

		bool		FetchNewMessage();

		uint8_t		GetSlaveID() const { return fBuf[3]; }
		uint8_t		GetMessageType() const { return fBuf[4]; }

	protected:

		CircularBuffer fBuf;

};
