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

std::optional<std::function<void(const std::shared_ptr<ProcessManager::Message>)>> ProcessManager::MessageHandler::Handle(const std::shared_ptr<Message> msg) const
{
	count++;
	if (msg->GetMessageTypeID() == 0)
		return {};
	auto f = find(msg->GetMessageTypeID());
	if (f != end())
		return f->second;
	else
		return { [](const std::shared_ptr<ProcessManager::Message>) {} };
}

int ProcessManager::MessageHandler::GetCount() const
{
	return count;
}

void ProcessManager::MessageHandler::Reset()
{
	count = 0;
}

ProcessManager::Process::Process(int PID, std::queue<std::shared_ptr<Message>>& incoming_messages, std::mutex& mtx,
	std::function<void(std::shared_ptr<Message>)> sendMessage, const MessageHandler& msgHandler)
	:
	PID(PID),
	mtx(mtx),
	incoming_messages(incoming_messages),
	sendMessage(sendMessage),
	msgHandler(msgHandler)
{}

int ProcessManager::Process::GetPID() const
{
	return PID;
}

void ProcessManager::Process::operator()()
{
	int prev_msg_id = -1;
	while (true)
	{
		std::optional<std::function<void(const std::shared_ptr<ProcessManager::Message>)>> f;
		if (!incoming_messages.empty())
		{
			auto msg = incoming_messages.front();
			if (msg->GetID() != prev_msg_id)
			{
				prev_msg_id = msg->GetID();
				// temp scope so the lock guard is destroyed 
				// before the the function is called
				{
					std::lock_guard<std::mutex> g(mtx);
					if (!(f = msgHandler.Handle(msg)))
						return;
				}
				f.value()(msg);
			}
		}
		Sleep(2);
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
		Sleep(10);
	}
	for (auto& t : threads)
		if (t.joinable())
			t.join();
}

void ProcessManager::AddProcess()
{
	const int id = threads.size() + 1;
	auto sendMsg = [this, id](std::shared_ptr<Message> msg) 
	{
		if (msg->GetMessageTypeID() == 0 || msg->GetSenderID() != id)
			return;
		msgLine.push(msg);
	};
	threads.push_back(std::thread(Process(id, msgLine, mtx, std::move(sendMsg), msgHandler)));
}

void ProcessManager::Update()
{
	if (!msgLine.empty() && msgHandler.GetCount() >= threads.size())
	{
		msgHandler.Reset();
		msgLine.pop();
	}
}

void ProcessManager::PostQuitMessage()
{
	msgLine.push(std::make_shared<Message>(0, 0));
}