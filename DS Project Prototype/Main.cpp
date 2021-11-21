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

std::vector<int> prime_factorisation(int num)
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
	ProcessManager pm(5);

	pm.AddHandlerFunction(typeid(EchoMessage<std::string>), 
		[](std::shared_ptr<ProcessManager::Message> msg_in) 
		{
			auto msg = (EchoMessage<std::string>*)msg_in.get();
			std::ostringstream oss;
			oss << msg->GetMessageString();
			std::cout << oss.str();
		}
	);
	pm.AddHandlerFunction(typeid(PocessableMessage<int>), 
		[](std::shared_ptr<ProcessManager::Message> msg_in) 
		{
			auto msg = (PocessableMessage<int>*)msg_in.get();
			std::ostringstream oss;
			auto res = prime_factorisation(msg->GetPayload());
			for (int i = 0; i < res.size(); i++)
				oss << res[i] << ' ';
			oss << std::endl;
			std::cout << oss.str();
		}
	);
	pm.AddHandlerFunction(typeid(SleepMessage),
		[](std::shared_ptr<ProcessManager::Message> msg_in)
		{
			auto msg = (SleepMessage*)msg_in.get();
			Sleep(msg->GetDuration());
		}
	);

	pm.BroadcastMessage(std::make_shared<PocessableMessage<int>>(0, 20));
	pm.BroadcastMessage(std::make_shared<SleepMessage>(0, 1000));
	return 0;
}