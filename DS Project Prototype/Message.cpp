#include "Message.h"

size_t Blockchain::Message::count = 0;

Blockchain::Message::Message(size_t senderID)
	:
	senderID(senderID),
	ID(++count)
{
}

size_t Blockchain::Message::GetID() const
{
	return ID;
}

size_t Blockchain::Message::GetSenderID() const
{
	return senderID;
}
