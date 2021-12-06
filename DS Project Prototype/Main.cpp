#include <iostream>
#include <sstream>
#include "NetworkManager.h"
#include "Puzzles.h"
#include "Block.h"
#include <unordered_map>
#include <fstream>

using namespace Blockchain;

int main(void)
{
	srand(time(0));
	NetworkManager manager(5);

	manager.AddMessageHandler(typeid(HashPuzzle2), 
		[](size_t minerID, MsgPtr puzzle_in)
		{
			auto puzzle = (HashPuzzle2*)puzzle_in.get();
			auto block = puzzle->GetBlock();
			
			size_t i = 0; // nonce
			while (true)
			{
				block.Mine(minerID, i);
				if (puzzle->Verify(block))
					break;
				i++;
			}

			return block;
		}
	);

	Block b;
	std::ifstream in("Input.txt");
	size_t hash = 0;
	while (in.peek() != EOF)
	{
		in >> b;
		hash = manager.MineBlock<HashPuzzle2>(b);
	}

	auto blocks = manager.GetBlocks(
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