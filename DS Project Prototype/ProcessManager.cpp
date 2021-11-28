#include "ProcessManager.h"
#include <iostream>
#include <cassert>

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

std::optional<ProcessManager::Callable> ProcessManager::MessageHandler::Handle(const ProcessManager::MsgPtr msg) const
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
		return { [](const ProcessManager::MsgPtr) { return std::string(); } }; // might throw exception later
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

void ProcessManager::MessageHandler::AddFunc(std::type_index msg_id, Callable func)
{
	funcMap.insert_or_assign(msg_id, std::move(func));
}

ProcessManager::Process::Process(size_t PID, const std::queue<ProcessManager::MsgPtr>& incoming_messages, std::mutex& mtx, std::mutex& wMtx,
	ProcessManager::Callable sendMessage, const MessageHandler& msgHandler)
	:
	PID(PID),
	mtx(mtx),
	wMtx(wMtx),
	incoming_messages(incoming_messages),
	sendMessage(sendMessage),
	msgHandler(msgHandler),
	t([this]() { func(); })
{
	std::ostringstream oss;
	oss << "process-" << PID << ".txt";
	filename = oss.str(); // assign a filename to the process
	std::ofstream(filename, std::ios::app).close(); // make a new file if it does not exist
}

ProcessManager::Process::~Process()
{
	if (t.joinable())
		t.join();
}

size_t ProcessManager::Process::GetPID() const
{
	return PID;
}

void ProcessManager::Process::func()
{
	size_t prev_msg_id = 0;
	while (true)
	{
		std::optional<ProcessManager::Callable> f;
		if (!incoming_messages.empty())
		{
			auto msg = incoming_messages.front();
			if (msg->GetID() != prev_msg_id && msg->GetSenderID() != PID)
			{
				s = State::Running;
				prev_msg_id = msg->GetID();
				// temp scope so the lock guard is destroyed 
				// before the the function is called
				{
					std::lock_guard<std::mutex> g(mtx);
					if (!(f = msgHandler.Handle(msg)))
					{
						s = State::Terminated;
						return;
					}
				}
				auto res = f.value()(msg);
				if (res)
				{
					std::lock_guard<std::mutex> g(wMtx);
					sendMessage(std::make_shared<Response>(PID, res.value()));
				}
				s = State::Waiting;
			}
		}
		Sleep(2);
	}
}

ProcessManager::ProcessManager(size_t num_processes)
	:
	msgHandler(msgLine, std::type_index(typeid(QuitMessage)))
{
	assert(num_processes != 0);
	AddProcesses(num_processes);
}

ProcessManager::~ProcessManager()
{
	PostQuitMessage();
	// wait for all messages to be processed
	WaitForCompletion();
}

void ProcessManager::AddHandlerFunction(std::type_index msg_id, Callable func)
{
	msgHandler.AddFunc(msg_id, std::move(func));
}

bool ProcessManager::Completed() const
{
	for (auto& p : processes)
	{
		if (p->Running())
			return false;
	}
	return msgLine.empty();
}

void ProcessManager::WaitForCompletion() const
{
	while (!Completed())
		Sleep(10);
}

bool ProcessManager::ResultsAreAvailable() const
{
	return !processResults.empty();
}

ProcessManager::MsgPtr ProcessManager::GetFirstResponse()
{
	assert(ResultsAreAvailable());
	auto r = processResults.front();
	processResults.pop();
	return r;
}

void ProcessManager::AddProcess(size_t id)
{
	auto sendMsg = [this, id](MsgPtr msg) 
	{
		if (msg->GetSenderID() != id) // to prevent accidentally sending messages from dif threads
			return std::optional<std::string>(); // might throw exception later
		processResults.push(msg);
		return std::optional<std::string>();
	};
	processes.emplace_back(std::make_unique<Process>(id, msgLine, mtx, wMtx, std::move(sendMsg), msgHandler));
	msgHandler.SetNumProcesses(processes.size());
}

void ProcessManager::AddProcesses(size_t num_processes)
{
	for (size_t i = 0; i < num_processes; i++)
		AddProcess(i + 1);
}

void ProcessManager::PostQuitMessage()
{
	msgLine.push(std::make_shared<QuitMessage>());
}
