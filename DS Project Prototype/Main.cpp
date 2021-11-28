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

	// passing some test data to the process
	nlohmann::json block;

	block["owner"] = "sarim"; // the owner of this data NOT the minor who solved the puzzle
	block["ownerID"] = "12345"; // any unique random id
	block["msg"] = "hi";
	pm.MineBlock(std::make_shared<PocessableMessage<int>>(20), block);
	/*=================*/
	block["owner"] = "ali"; // the owner of this data NOT the minor who solved the puzzle
	block["ownerID"] = "12346"; // any unique random id
	block["msg"] = "hi";
	pm.MineBlock(std::make_shared<PocessableMessage<int>>(20), block);
	/*=================*/
	block["owner"] = "sarim"; // the owner of this data NOT the minor who solved the puzzle
	block["ownerID"] = "12345"; // any unique random id
	block["msg"] = "bye";
	pm.MineBlock(std::make_shared<PocessableMessage<int>>(20), block);
	/*=================*/
	block["owner"] = "ali"; // the owner of this data NOT the minor who solved the puzzle
	block["ownerID"] = "12346"; // any unique random id
	block["msg"] = "bye";
	pm.MineBlock(std::make_shared<PocessableMessage<int>>(20), block);
	/*=================*/

	auto arr = pm.GetBlock(
		[](const nlohmann::json& obj) 
		{ 
			return obj["ownerID"] == "12346"; 
		}
	);

	for (const auto& obj : arr)
	{
		std::cout << obj << std::endl;
	}

	return 0;
}