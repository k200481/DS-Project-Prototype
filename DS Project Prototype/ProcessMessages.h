#pragma once
#include "ProcessManager.h"
template <typename T>
class EchoMessage : public ProcessManager::Message
{
public:
	EchoMessage(size_t senderID, const T& msg)
		:
		ProcessManager::Message(std::type_index(typeid(EchoMessage)), senderID),
		msg(msg)
	{}

	const T& GetMessageString() const
	{
		return msg;
	}
private:
	T msg;
};

template <typename T>
class PocessableMessage : public ProcessManager::Message
{
public:
	PocessableMessage(size_t senderID, T msg)
		:
		ProcessManager::Message(std::type_index(typeid(PocessableMessage)), senderID),
		msg(msg)
	{}

	const T& GetPayload() const
	{
		return msg;
	}

private:
	const T msg;
};

class SleepMessage : public ProcessManager::Message
{
public:
	SleepMessage(size_t senderID, size_t duration)
		:
		ProcessManager::Message(std::type_index(typeid(SleepMessage)), senderID),
		duration(duration)
	{}
	size_t GetDuration()
	{
		return duration;
	}
private:
	const size_t duration;
};