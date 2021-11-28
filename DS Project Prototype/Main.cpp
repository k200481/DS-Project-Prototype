#include <iostream>
#include <sstream>
#include "ProcessManager.h"
#include "ProcessMessages.h"

int fun(int num)
{
	if (num <= 1)
		return 1;
	return fun(num - 1) + fun(num - 2);
}

std::vector<int> integer_factorisation(int num)
{
	std::vector<int> factors;
	for (int i = 0; i < num; i++)
	{
		auto n = float(num) / i;
		if ((n - int(n)) == 0.0f)
		{
			factors.push_back(i);
		}
	}
	factors.push_back(num);
	return factors;
}

int main(void)
{
	std::queue<ProcessManager::MsgPtr> q;
	ProcessManager pm(q, 5);

	pm.AddHandlerFunction(typeid(EchoMessage<int>),
		[](ProcessManager::MsgPtr msg_in)
		{
			std::ostringstream oss;
			oss << ((EchoMessage<int>*)msg_in.get())->GetMessageString();
			return oss.str();
		}
	);
	pm.AddHandlerFunction(typeid(PocessableMessage<int>), 
		[](ProcessManager::MsgPtr msg_in)
		{
			auto msg = (PocessableMessage<int>*)msg_in.get();
			std::ostringstream oss;
			auto res = integer_factorisation(msg->GetPayload());
			for (size_t i = 0; i < res.size(); i++)
				oss << res[i] << ' ';
//			oss << std::endl;
			//std::cout << oss.str();
			return oss.str();
		}
	);
	pm.AddHandlerFunction(typeid(SleepMessage),
		[](ProcessManager::MsgPtr msg_in)
		{
			auto msg = (SleepMessage*)msg_in.get();
			Sleep(msg->GetDuration());
			return std::optional<std::string>{};
		}
	);

	pm.BroadcastMessage(std::make_shared<PocessableMessage<int>>(20));
	while (!pm.Completed())
	{
		Sleep(10);
	}

	std::cout << "Processing Results\n";
	while (!q.empty())
	{
		auto f = q.front();
		q.pop();
		auto res = (ProcessManager::Response*)f.get();
		std::cout << res->GetSenderID() << ": " << res->GetResult() << std::endl;
	}
	
	pm.BroadcastMessage(std::make_shared<EchoMessage<int>>(20));

	while (!pm.Completed())
	{
		Sleep(10);
	}

	std::cout << "\nEcho response\n";
	while (!q.empty())
	{
		auto f = q.front();
		q.pop();
		auto res = (ProcessManager::Response*)f.get();
		std::cout << "From Process " << res->GetSenderID() << ": " << res->GetResult() << std::endl;
	}

	return 0;
}