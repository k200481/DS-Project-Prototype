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

std::optional<ProcessManager::Callable> ProcessManager::MessageHandlerMap::GetMessageHandler(const ProcessManager::MsgPtr msg) const
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
		throw std::exception("Invalid Message typeid received");
}

ProcessManager::MessageHandlerMap::MessageHandlerMap(std::queue<std::shared_ptr<Message>>& msgLine, std::type_index quit_id)
	:
	msgLine(msgLine),
	quit_id(quit_id)
{
}

void ProcessManager::MessageHandlerMap::SetNumProcesses(size_t num_processes)
{
	this->num_processes = num_processes;
}

void ProcessManager::MessageHandlerMap::AddFunc(std::type_index msg_id, Callable func)
{
	funcMap.insert_or_assign(msg_id, std::move(func));
}

ProcessManager::Miner::Miner(size_t PID, const std::queue<ProcessManager::MsgPtr>& incoming_messages, std::mutex& mtx, std::mutex& wMtx,
	std::function<std::optional<std::string>(const MsgPtr)> sendMessage, const MessageHandlerMap& msgHandler)
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

ProcessManager::Miner::~Miner()
{
	if (t.joinable())
		t.join();
}

size_t ProcessManager::Miner::GetPID() const
{
	return PID;
}

void ProcessManager::Miner::SaveBlock(const nlohmann::json& j)
{
	std::ofstream out(filename, std::ios::app);
	out << std::endl << std::setw(4) << j;
}

void ProcessManager::Miner::func()
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
					if (!(f = msgHandler.GetMessageHandler(msg)))
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

void ProcessManager::AddMessageHandler(std::type_index msg_id, Callable func)
{
	msgHandler.AddFunc(msg_id, std::move(func));
}

void ProcessManager::BroadcastMessage(MsgPtr msg)
{
	msgLine.push(msg);
}

bool ProcessManager::Completed() const
{
	for (auto& p : miners)
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

bool ProcessManager::ResponsesAreAvailable() const
{
	return !processResults.empty();
}

ProcessManager::MsgPtr ProcessManager::GetFirstResponse()
{
	assert(ResponsesAreAvailable());
	auto r = processResults.front();
	processResults.pop();
	return r;
}

void ProcessManager::SaveBlock(size_t PID, nlohmann::json j)
{
	for (auto& p : miners)
	{
		if (p->GetPID() == PID)
		{
			p->SaveBlock(j);
			break;
		}
	}
}

size_t ProcessManager::MineBlock(MsgPtr msg)
{
	if (ResponsesAreAvailable())
		throw std::exception("There were unread responses in the queue when Mineblock was called");

	BroadcastMessage(msg);
	WaitForCompletion();

	int verifications = 0;
	auto f = GetFirstResponse();
	auto res = ((Response*)(f.get()))->GetBlock();
	while (ResponsesAreAvailable())
	{
		auto temp = GetFirstResponse();
		if (res.GetHash() == ((Response*)(temp.get()))->GetBlock().GetHash())
		{
			verifications++; // signifies the number of processes that got the same hash
			// the hash may be correct even if other processes got a different one
		}
	}

	auto block = res.GetJSON();
	block["verified-by"] = verifications;
	block["total miners"] = miners.size();
	block["miner"] = f->GetSenderID();
	SaveBlock(f->GetSenderID(), block);
	return block["hash"];
}

void ProcessManager::AddProcess(size_t id)
{
	auto sendMsg = [this, id](MsgPtr msg) 
	{
		if (msg->GetSenderID() != id) // to prevent accidentally sending messages from dif threads
			throw std::exception("Incorrect Sender ID");
		processResults.push(msg);
		return std::optional<std::string>();
	};
	miners.emplace_back(std::make_unique<Miner>(id, msgLine, mtx, wMtx, std::move(sendMsg), msgHandler));
	msgHandler.SetNumProcesses(miners.size());
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
