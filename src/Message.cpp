#include "Message.h"
#include "Buffer.h"

Message::Message(Buffer&& buffer) : fBuffer(buffer), fType((Message::Type)buffer[3]) {}