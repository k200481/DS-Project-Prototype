#include "Miner.h"
#include <sstream>
#include "Block.h"
#include <windows.h>
#include "NetworkManager.h"

using namespace Blockchain;

Miner::Miner(size_t PID, std::mutex& mtx, std::mutex& wMtx,
	std::function<void(const MsgPtr)> sendMessage,
	std::function<std::optional<MsgPtr>(size_t)> getMessage,
	const std::unordered_map<std::type_index, Callable>& msgHandlerMap
)
	:
	PID(PID),
	mtx(mtx),
	wMtx(wMtx),
	getMessage(getMessage),
	sendMessage(sendMessage),
	msgHandlerMap(msgHandlerMap),
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
	while (true)
	{
		std::optional<MsgPtr> msg;
		Callable f;
		// temp scope so lock guard is destroyed immediatlely
		{
			std::lock_guard g(mtx);
			msg = getMessage(PID);
		}
		if (msg.has_value())
		{
			s = State::Running;
			if (msg.value()->GetTypeID() == typeid(QuitMessage))
			{
				s = State::Terminated;
				return;
			}
			f = msgHandlerMap.at(msg.value()->GetTypeID());
			
			auto res = f(PID, msg.value());
			if (res)
			{
				std::lock_guard<std::mutex> g(wMtx);
				sendMessage(std::make_shared<Solution>(PID, res.value()));
			}
			s = State::Waiting;
		}
		Sleep(2);
	}
}
