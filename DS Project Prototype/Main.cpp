#include <iostream>
#include "ProcessManager.h"

int main(void)
{
	ProcessManager::MessageHandler mh;
	mh.insert({ 1, [](const std::unique_ptr<ProcessManager::Message>& msg) { std::cout << "Message ID: " << msg->GetID() << std::endl; }});

	ProcessManager pm(std::move(mh));
	pm.AddProcess();
	pm.AddProcess();
	pm.AddProcess();
	pm.AddProcess();

	pm.BroadcastMessage(std::make_unique<ProcessManager::Message>(1, 0));
	pm.BroadcastMessage(std::make_unique<ProcessManager::Message>(1, 0));
	pm.BroadcastMessage(std::make_unique<ProcessManager::Message>(1, 0));
	pm.BroadcastMessage(std::make_unique<ProcessManager::Message>(1, 0));
	pm.BroadcastMessage(std::make_unique<ProcessManager::Message>(1, 0));

	return 0;
}