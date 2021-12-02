#include <iostream>
#include <sstream>
#include "ProcessManager.h"
#include "ProcessMessages.h"
#include "Block.h"
#include <unordered_map>

int main(void)
{
	srand(time(0));
	ProcessManager pm(5);

	pm.AddMessageHandler(typeid(HashPuzzle1), 
		[](ProcessManager::MsgPtr puzzle_in)
		{
			auto puzzle = (HashPuzzle1*)puzzle_in.get();
			auto block = puzzle->GetBlock();
			size_t res;
			std::hash<std::string> str_hasher;
			
			size_t i = 0; // nonce
			while (true)
			{
				std::ostringstream oss;
				oss << block.GetMsg() << block.GetOwnerName() << block.GetMsg() << i;

				res = str_hasher(oss.str());
				if (puzzle->Verify(res))
					break;
				i = rand();
			}
			block.UpdateMiningInfo(res, i);
			return block;
		}
	);

	Block b;
	std::ifstream in("Input.txt");
	size_t hash = 0;
	while (in.peek() != EOF)
	{
		b.ReadFromJSON(in);
		hash = pm.MineBlock(MakePuzzle<HashPuzzle1>(b));
	}

	auto blocks = pm.GetBlocks(
		[](const nlohmann::json& j)
		{
			return j["ownerID"] == "20K-0481";
		}
	);

	for (auto& b : blocks)
	{
		std::cout << std::setw(4) << b << std::endl;
	}

	return 0;
}