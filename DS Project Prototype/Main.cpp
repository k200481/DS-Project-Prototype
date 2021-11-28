#include <iostream>
#include <sstream>
#include "ProcessManager.h"
#include "ProcessMessages.h"

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
	ProcessManager pm(5);

	pm.AddHandlerFunction(typeid(PocessableMessage<int>), 
		[](ProcessManager::MsgPtr msg_in)
		{
			auto msg = (PocessableMessage<int>*)msg_in.get();
			std::ostringstream oss;
			auto res = integer_factorisation(msg->GetPayload());
			for (size_t i = 0; i < res.size(); i++)
				oss << res[i] << ' ';
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

	pm.BroadcastMessage(std::make_shared<PocessableMessage<int>>(10000000));
	pm.WaitForCompletion();

	std::cout << "Processing Results\n";
	while (pm.ResultsAreAvailable())
	{
		auto f = pm.GetFirstResponse();
		auto res = (ProcessManager::Response*)f.get();
		std::cout << res->GetSenderID() << ": " << res->GetResult() << std::endl;
	}

	return 0;
}