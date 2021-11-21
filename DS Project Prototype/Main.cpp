#include <iostream>
#include <sstream>
#include "ProcessManager.h"

int fun(int num)
{
	if (num <= 1)
		return 1;
	return fun(num - 1) + fun(num - 2);
}

int main(void)
{
	class EchoMessage : public ProcessManager::Message
	{
	public:
		EchoMessage(int senderID, const char* msg)
			:
			ProcessManager::Message(std::type_index(typeid(EchoMessage)), senderID),
			msg(msg)
		{}

		const char* GetMessageString() const
		{
			return msg;
		}
	private:
		const char* msg;
	};

	class PocessableMessage : public ProcessManager::Message
	{
	public:
		PocessableMessage(int senderID, int num)
			:
			ProcessManager::Message(std::type_index(typeid(PocessableMessage)), senderID),
			num(num)
		{}

		int GetNum() const
		{
			return num;
		}

	private:
		const int num;
	};

	class SleepMessage : public ProcessManager::Message
	{
	public:
		SleepMessage(int senderID)
			:
			ProcessManager::Message(std::type_index(typeid(SleepMessage)), senderID)
		{}
	};

	ProcessManager pm(20);

	pm.AddHandlerFunction(typeid(EchoMessage), 
		[](std::shared_ptr<ProcessManager::Message> msg_in) 
		{
			auto msg = (EchoMessage*)msg_in.get();
			std::ostringstream oss;
			oss << msg->GetMessageString();
			std::cout << oss.str();
		}
	);
	pm.AddHandlerFunction(typeid(PocessableMessage), 
		[](std::shared_ptr<ProcessManager::Message> msg_in) 
		{
			auto msg = (PocessableMessage*)msg_in.get();
			std::ostringstream oss;
			oss << fun(msg->GetNum()) << std::endl;
			std::cout << oss.str();
		}
	);
	pm.AddHandlerFunction(typeid(SleepMessage),
		[](std::shared_ptr<ProcessManager::Message> msg_in)
		{
			Sleep(100);
		}
	);

	pm.BroadcastMessage(std::make_shared<EchoMessage>(0, "Hi\n"));
	pm.BroadcastMessage(std::make_shared<PocessableMessage>(0, 20));
	pm.BroadcastMessage(std::make_shared<EchoMessage>(0, "Bye\n"));
	return 0;
}