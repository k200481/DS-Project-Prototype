#include "ProcessManager.h"

int ProcessManager::Message::count = 0;
ProcessManager::Message::Message(int typeID, int senderID)
	:
	messageTypeID(typeID),
	senderID(senderID),
	ID(count++)
{
}

int ProcessManager::Message::GetID() const
{
	return ID;
}

int ProcessManager::Message::GetMessageTypeID() const
{
	return messageTypeID;
}

int ProcessManager::Message::GetSenderID() const
{
	return senderID;
}

std::optional<std::function<void(const std::unique_ptr<ProcessManager::Message>&)>> ProcessManager::MessageHandler::Handle(const std::unique_ptr<Message>& msg) const
{
	count++;
	if (msg->GetMessageTypeID() == 0 && msg->GetSenderID() == 0)
		return {};
	else
		return find(msg->GetMessageTypeID())->second;
}

int ProcessManager::MessageHandler::GetCount() const
{
	return count;
}

void ProcessManager::MessageHandler::Reset()
{
	count = 0;
}

ProcessManager::Process::Process(int PID, std::queue<std::unique_ptr<Message>>& incoming_messages, std::mutex& mtx,
	std::function<void(std::unique_ptr<Message>)> sendMessage, const MessageHandler& msgHandler)
	:
	PID(PID),
	mtx(mtx),
	incoming_messages(incoming_messages),
	sendMessage(sendMessage),
	msgHandler(msgHandler)
{}

void ProcessManager::Process::operator()()
{
	int prev_msg_id = -1;
	std::optional<std::function<void(const std::unique_ptr<ProcessManager::Message>&)>> f;
	while (true)
	{
		// temp scope so the lock guard is destroyed before the process goes to sleep
		{
			if (!incoming_messages.empty())
			{
				const auto& msg = incoming_messages.front();
				if (msg->GetID() != prev_msg_id)
				{
					prev_msg_id = msg->GetID();
					std::lock_guard<std::mutex> g(mtx);
					if (!(f = msgHandler.Handle(msg)))
						return;
				}
				f.value()(msg);
			}
		}
		Sleep(10);
	}
}

ProcessManager::ProcessManager(ProcessManager::MessageHandler msgHandler)
	:
	msgHandler(std::move(msgHandler))
{}

ProcessManager::~ProcessManager()
{
	PostQuitMessage();
	while (!msgLine.empty())
	{
		Update();
		Sleep(100);
	}
	for (auto& t : threads)
		if (t.joinable())
			t.join();
}

void ProcessManager::AddProcess()
{
	auto sendMsg = [this](std::unique_ptr<Message> msg) 
	{
		msgLine.push(std::move(msg));
	};
	threads.push_back(std::thread(Process(threads.size() + 1, msgLine, mtx, sendMsg, msgHandler)));
}

void ProcessManager::Update()
{
	if (!msgLine.empty() && msgHandler.GetCount() == threads.size())
	{
		msgHandler.Reset();
		msgLine.pop();
	}
}

void ProcessManager::PostQuitMessage()
{
	msgLine.push(std::make_unique<Message>(0, 0));
}