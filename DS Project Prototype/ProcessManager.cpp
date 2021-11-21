#include "ProcessManager.h"

size_t ProcessManager::Message::count = 0;
ProcessManager::Message::Message(std::type_index typeID, size_t senderID)
	:
	messageTypeID(typeID),
	senderID(senderID),
	ID(++count)
{
}

size_t ProcessManager::Message::GetID() const
{
	return ID;
}

std::type_index ProcessManager::Message::GetMessageTypeID() const
{
	return messageTypeID;
}

size_t ProcessManager::Message::GetSenderID() const
{
	return senderID;
}

std::optional<std::function<void(const std::shared_ptr<ProcessManager::Message>)>> ProcessManager::MessageHandler::Handle(const std::shared_ptr<Message> msg) const
{
	count++;
	if (num_processes == count)
	{
		count = 0;
		msgLine.pop();
	}

	if (msg->GetMessageTypeID() == quit_id)
		return {};
	
	auto f = funcMap.find(msg->GetMessageTypeID());
	if (f != funcMap.end())
		return f->second;
	else
		return { [](const std::shared_ptr<ProcessManager::Message>) {} }; // might throw exception later
}

ProcessManager::MessageHandler::MessageHandler(std::queue<std::shared_ptr<Message>>& msgLine, std::type_index quit_id)
	:
	msgLine(msgLine),
	quit_id(quit_id)
{
}

void ProcessManager::MessageHandler::SetNumProcesses(size_t num_processes)
{
	this->num_processes = num_processes;
}

void ProcessManager::MessageHandler::AddFunc(std::type_index msg_id, std::function<void(const std::shared_ptr<Message>)> func)
{
	funcMap.insert({ msg_id, func });
}

ProcessManager::Process::Process(size_t PID, std::queue<std::shared_ptr<Message>>& incoming_messages, std::mutex& mtx,
	std::function<void(std::shared_ptr<Message>)> sendMessage, const MessageHandler& msgHandler)
	:
	PID(PID),
	mtx(mtx),
	incoming_messages(incoming_messages),
	sendMessage(sendMessage),
	msgHandler(msgHandler)
{}

size_t ProcessManager::Process::GetPID() const
{
	return PID;
}

void ProcessManager::Process::operator()()
{
	size_t prev_msg_id = 0;
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
				if(msg->GetSenderID() != PID) // so the thread doesn't act on its own messages
					f.value()(msg);
			}
		}
		Sleep(2);
	}
}

ProcessManager::ProcessManager(size_t num_processes)
	:
	msgHandler(msgLine, std::type_index(typeid(QuitMessage)))
{
	AddProcesses(num_processes);
}

ProcessManager::~ProcessManager()
{
	PostQuitMessage();
	while (!msgLine.empty())
	{
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
		if (msg->GetSenderID() != id) // to prevent accidentally sending messages from dif threads
			return; // might throw exception later
		msgLine.push(msg);
	};
	threads.push_back(std::thread(Process(id, msgLine, mtx, std::move(sendMsg), msgHandler)));
	msgHandler.SetNumProcesses(threads.size());
}

void ProcessManager::AddProcesses(size_t num_processes)
{
	for (size_t i = 0; i < num_processes; i++)
		AddProcess();
}

void ProcessManager::PostQuitMessage()
{
	msgLine.push(std::make_shared<QuitMessage>());
}

void ProcessManager::AddHandlerFunction(std::type_index msg_id, std::function<void(const std::shared_ptr<Message>)> func)
{
	msgHandler.AddFunc(msg_id, std::move(func));
}