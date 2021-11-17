#include <iostream>
#include "ProcessManager.h"

int main(void)
{
	class EchoMessage : public ProcessManager::Message
	{
	public:
		EchoMessage(int typeID, int senderID, const char* msg)
			:
			ProcessManager::Message(typeID, senderID),
			msg(msg)
		{}

		const char* GetMessageString() const
		{
			return msg;
		}
	private:
		const char* msg;
	};

	ProcessManager::MessageHandler mh;
	mh.insert({ mh.size() + 1, [](const std::unique_ptr<ProcessManager::Message>& msg) { std::cout << "Message ID: " << msg->GetID() << std::endl; } });
	mh.insert({ mh.size() + 1, [](const std::unique_ptr<ProcessManager::Message>& msg) { std::cout << ((EchoMessage*)msg.get())->GetMessageString() << std::endl; }});


	ProcessManager pm(std::move(mh));
	pm.AddProcess();
	pm.AddProcess();
	pm.AddProcess();
	pm.AddProcess();

	pm.BroadcastMessage(std::make_unique<EchoMessage>(2, 0, "Hi"));
	pm.BroadcastMessage(std::make_unique<EchoMessage>(2, 0, "Bye"));

	return 0;
}