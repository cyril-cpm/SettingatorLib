#pragma once

#include "Buffer.h"
#include <cstdint>
#include <variant>

class ICore
{
	public:

		bool		FetchNewMessage();

		uint8_t		GetSrcSlaveID() const { return fBuf[3]; }
		uint8_t		GetDstSlaveID() const { return fBuf[4]; }
		uint8_t		GetMessageType() const { return fBuf[5]; }

	protected:

		CircularBuffer fBuf;

};



