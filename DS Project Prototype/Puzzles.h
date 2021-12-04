#pragma once
#include "Message.h"
#include "Block.h"

namespace Blockchain
{
	// the base puzzle class
	class Puzzle : public Message
	{
	public:
		Puzzle(const Block& b)
			:
			Message(0u),
			b(b)
		{}
		// get the block to be mined
		Block GetBlock() const
		{
			return b;
		}
		// check if a givn block satisfies the constraints of the puzzle
		virtual bool Verify(const Block& mined_block) const = 0;
	private:
		Block b;
	};

	class HashPuzzle1 : public Puzzle
	{
	public:
		HashPuzzle1(const Block& block)
			:
			Puzzle(block)
		{
			modResReq = rand() % 100;
			modVal = 1 + modResReq + rand() % 2000;
		}
		virtual std::type_index GetTypeID() const override
		{
			return typeid(HashPuzzle1);
		}
		// used to verify if a hash satisfies the given conditions
		virtual bool Verify(const Block& mined_block) const override
		{
			return mined_block.VerifyIntegrity() && (mined_block.GetHash() % modVal) == modResReq;
		}
	private:
		unsigned int modVal;
		unsigned int modResReq;
	};

	class HashPuzzle2 : public Puzzle
	{
	public:
		HashPuzzle2(const Block& block)
			:
			Puzzle(block)
		{
		}
		virtual std::type_index GetTypeID() const override
		{
			return typeid(HashPuzzle2);
		}
		// used to verify if a hash satisfies the given conditions
		virtual bool Verify(const Block& mined_block) const override
		{
			return mined_block.VerifyIntegrity() && (mined_block.GetHash() % 10) == 0;
		}
	};

	template <typename T1, typename T2>
	std::shared_ptr<T1> MakePuzzle(const T2& t)
	{
		return std::make_shared<T1>(t);
	}
}

