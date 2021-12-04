#include "Miner.h"
#include <sstream>
#include "Block.h"
#include <windows.h>
#include "NetworkManager.h"

using namespace Blockchain;

Miner::Miner(size_t PID, const std::queue<MsgPtr>& incoming_messages, std::mutex& mtx, std::mutex& wMtx,
	std::function<void(const MsgPtr)> sendMessage, const MessageHandlerMap& msgHandler)
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

Miner::~Miner()
{
	if (t.joinable())
		t.join();
}

size_t Miner::GetPID() const
{
	return PID;
}

bool Miner::Waiting() const
{
	return s == State::Waiting;
}

bool Miner::Terminated() const
{
	return s == State::Terminated;
}

bool Miner::Running() const
{
	return s == State::Running;
}

void Miner::SaveBlock(const nlohmann::json& j)
{
	std::ofstream out(filename, std::ios::app);
	out << std::endl << std::setw(4) << j;
}

void Miner::func()
{
	size_t prev_msg_id = 0;
	while (true)
	{
		std::optional<Callable> f;
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
				auto res = f.value()(PID, msg);
				if (res)
				{
					std::lock_guard<std::mutex> g(wMtx);
					sendMessage(std::make_shared<Solution>(PID, res.value()));
				}
				s = State::Waiting;
			}
		}
		Sleep(2);
	}
}
