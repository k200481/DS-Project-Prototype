#include "NetworkManager.h"
#include <iostream>
#include <cassert>
#include "Puzzles.h"

using namespace Blockchain;

MessageHandlerMap::MessageHandlerMap(std::queue<std::shared_ptr<Message>>& msgLine, std::type_index quit_id)
	:
	msgLine(msgLine),
	quit_id(quit_id)
{
}

std::optional<Callable> MessageHandlerMap::GetMessageHandler(const MsgPtr msg) const
{
	count++;
	if (numMiners == count)
	{
		count = 0;
		msgLine.pop();
	}

	if (msg->GetTypeID() == quit_id)
		return {};
	
	auto f = funcMap.find(msg->GetTypeID());
	if (f != funcMap.end())
		return f->second;
	else
		throw std::exception("Invalid Message typeid received");
}

void MessageHandlerMap::SetNumMiners(size_t numMiners)
{
	this->numMiners = numMiners;
}

void MessageHandlerMap::AddFunc(std::type_index msg_id, Callable func)
{
	funcMap.insert_or_assign(msg_id, std::move(func));
}



NetworkManager::NetworkManager(size_t num_processes)
	:
	msgHandler(msgLine, std::type_index(typeid(QuitMessage)))
{
	assert(num_processes != 0);
	AddProcesses(num_processes);
}

NetworkManager::~NetworkManager()
{
	PostQuitMessage();
	// wait for all messages to be processed
	WaitForCompletion();
}

void NetworkManager::AddMessageHandler(std::type_index msg_id, Callable func)
{
	msgHandler.AddFunc(msg_id, std::move(func));
}

void NetworkManager::BroadcastMessage(MsgPtr msg)
{
	msgLine.push(msg);
}

bool NetworkManager::Completed() const
{
	for (auto& p : miners)
	{
		if (p->Running())
			return false;
	}
	return msgLine.empty();
}

void NetworkManager::WaitForCompletion() const
{
	while (!Completed())
		Sleep(10);
}

bool NetworkManager::ResponsesAreAvailable() const
{
	return !processResults.empty();
}

MsgPtr NetworkManager::GetFirstResponse()
{
	assert(ResponsesAreAvailable());
	auto r = processResults.front();
	processResults.pop();
	return r;
}

void NetworkManager::SaveBlock(size_t PID, nlohmann::json j)
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

bool NetworkManager::MineBlock(MsgPtr msg)
{
	if (ResponsesAreAvailable())
		throw std::exception("There were unread responses in the queue when Mineblock was called");

	BroadcastMessage(msg);
	WaitForCompletion();

	int verifications = 0;
	auto f = GetFirstResponse();
	auto res = ((Solution*)(f.get()))->GetBlock();

	// retry until a valid block is obtained
	// probably not necessary
	while (!res.VerifyIntegrity() && ResponsesAreAvailable())
	{
		f = GetFirstResponse();
		res = ((Solution*)(f.get()))->GetBlock();
	}

	// this will probably never actually run
	// unless main really messes up
	if (!res.VerifyIntegrity())
	{
		std::cout << "No valid blocks were obtained\n";
		return false;
	}

	// verify other miners' responses
	while (ResponsesAreAvailable())
	{
		auto temp = GetFirstResponse();
		auto tempBlk = ((Solution*)(temp.get()))->GetBlock();
		if (tempBlk.VerifyIntegrity())
		{
			verifications++; // signifies the number of processes that were able to 
		}
	}

	auto block = res.GetJSON();
	block["verified-by"] = verifications;
	block["total miners"] = miners.size();
	SaveBlock(f->GetSenderID(), block);
	return true;
}

void NetworkManager::AddProcess(size_t id)
{
	auto sendMsg = [this, id](MsgPtr msg) 
	{
		if (msg->GetSenderID() != id) // to prevent accidentally sending messages from dif threads
			throw std::exception("Incorrect Sender ID");
		processResults.push(msg);
	};
	miners.emplace_back(std::make_unique<Miner>(id, msgLine, mtx, wMtx, std::move(sendMsg), msgHandler));
	msgHandler.SetNumMiners(miners.size());
}

void NetworkManager::AddProcesses(size_t num_processes)
{
	for (size_t i = 0; i < num_processes; i++)
		AddProcess(i + 1);
}

void NetworkManager::PostQuitMessage()
{
	msgLine.push(std::make_shared<QuitMessage>());
}
