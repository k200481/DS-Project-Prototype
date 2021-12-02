#pragma once
#include "ProcessManager.h"
#include "Block.h"

class HashPuzzle1 : public ProcessManager::Message
{
public:
	HashPuzzle1(const Block& block)
		:
		ProcessManager::Message(std::type_index(typeid(HashPuzzle1)), 0),
		block(block)
	{
		modResReq = rand() % 100;
		modVal = 1 + modResReq + rand() % 2000;
	}
	// returns the block passed to it
	Block GetBlock() const
	{
		return block;
	}
	// used to verify if a hash satisfies the given conditions
	bool Verify(size_t hash) const
	{
		return (hash % modVal) == modResReq;
	}
private:
	Block block;
	unsigned int modVal;
	unsigned int modResReq;
};

class HashPuzzle2 : public ProcessManager::Message
{
public:
	HashPuzzle2(const Block& block)
		:
		ProcessManager::Message(std::type_index(typeid(HashPuzzle2)), 0),
		block(block)
	{
	}
	// returns the block passed to it
	Block GetBlock() const
	{
		return block;
	}
	// used to verify if a hash satisfies the given conditions
	bool Verify(size_t hash) const
	{
		return (hash % 10) == 0;
	}
private:
	Block block;
};

template <typename T1, typename T2>
std::shared_ptr<T1> MakePuzzle(const T2& t)
{
	return std::make_shared<T1>(t);
}