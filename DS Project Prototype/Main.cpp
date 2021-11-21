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
			ProcessManager::Message(1, senderID),
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
			ProcessManager::Message(2, senderID),
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
			ProcessManager::Message(3, senderID)
		{}
	};

	ProcessManager::MessageHandler mh;

	mh.insert({ mh.size() + 1, [](const std::shared_ptr<ProcessManager::Message> msg)
		{
			EchoMessage new_msg(1, ((EchoMessage*)msg.get())->GetMessageString());
			std::ostringstream oss;
			oss << new_msg.GetSenderID() << ": " << new_msg.GetMessageString();
			std::cout << oss.str();
		}
		});
	mh.insert({ mh.size() + 1, [](const std::shared_ptr<ProcessManager::Message> msg)
		{
			int num = ((PocessableMessage*)msg.get())->GetNum();
			std::ostringstream oss;
			oss << fun(num) << std::endl;
			std::cout << oss.str();
		}
		});
	mh.insert({ mh.size() + 1, [](const std::shared_ptr<ProcessManager::Message> msg)
		{
			Sleep(100);
		}
	});

	ProcessManager pm(std::move(mh));
	pm.AddProcess();
	pm.AddProcess();

//	pm.BroadcastMessage(std::make_shared<EchoMessage>(0, "Hi\n"));
	pm.BroadcastMessage(std::make_shared<PocessableMessage>(0, 20));
//	pm.BroadcastMessage(std::make_shared<EchoMessage>(0, "Bye\n"));

	return 0;
}