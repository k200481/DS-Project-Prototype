#include "NetworkManager.h"
#include <iostream>
#include <cassert>
#include "Puzzles.h"

using namespace Blockchain;

NetworkManager::NetworkManager(size_t num_processes)
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
	msgHandlerMap.insert_or_assign(msg_id, std::move(func));
}

template <typename MsgT, typename DataT>
void NetworkManager::BroadcastMessage(DataT data)
{
	msgLine.push(std::make_shared<MsgT>(data));
}
template <typename MsgT>
void NetworkManager::BroadcastMessage()
{
	msgLine.push(std::make_shared<MsgT>());
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
	return !solutions.empty();
}

MsgPtr NetworkManager::GetFirstResponse()
{
	assert(ResponsesAreAvailable());
	auto r = solutions.front();
	solutions.pop();
	return r;
}

std::optional<MsgPtr> Blockchain::NetworkManager::MessageHandler(size_t PID)
{
	if (msgLine.empty() || msgReadBy[PID - 1])
	{
		return {};
	}
	msgReadBy[PID - 1] = true;

	auto f = msgLine.front();
	readCount++;
	if (readCount == miners.size())
	{
		msgLine.pop();
		msgReadBy.flip();
		readCount = 0;
	}
	return f;
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

template <typename MsgT>
bool NetworkManager::MineBlock(const Block& block)
{
	if (ResponsesAreAvailable())
		throw std::exception("There were unread responses in the queue when Mineblock was called");

	BroadcastMessage<MsgT>(block);
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

	auto mined_block = res.GetJSON();
	mined_block["verified-by"] = verifications;
	mined_block["total miners"] = miners.size();
	SaveBlock(f->GetSenderID(), mined_block);
	return true;
}

template bool NetworkManager::MineBlock<HashPuzzle1>(const Block&);
template bool NetworkManager::MineBlock<HashPuzzle2>(const Block&);

void NetworkManager::AddProcess(size_t id)
{
	auto sendMsg = [this, id](MsgPtr msg)
	{
		if (msg->GetSenderID() != id) // to prevent accidentally sending messages from dif threads
			throw std::exception("Incorrect Sender ID");

		solutions.push(msg);
	};
	miners.emplace_back(
		std::make_unique<Miner>(
			id, mtx, wMtx, std::move(sendMsg),
			[this](size_t PID)->std::optional<MsgPtr> { return MessageHandler(PID); },
			msgHandlerMap
		)
	);
	msgReadBy.emplace_back(false);
}

void NetworkManager::AddProcesses(size_t num_processes)
{
	for (size_t i = 0; i < num_processes; i++)
		AddProcess(i + 1);
}

void NetworkManager::PostQuitMessage()
{
	msgLine.push(std::make_shared<Miner::QuitMessage>());
}
