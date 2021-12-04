#pragma once
#include "nlohmann.h"
#include <sstream>

namespace Blockchain
{
	// a single block in the blockchain
	class Block
	{
	public:
		Block() = default;
		Block(size_t prevHash, std::string ownerID, std::string ownerName, std::string msg)
			:
			ownerID(ownerID),
			ownerName(ownerName),
			msg(msg)
		{}
		// returns a JSON object constructed from the block
		nlohmann::json GetJSON() const;
		// read from input stream in JSON format
		friend std::istream& operator>>(std::istream& in, Block& b)
		{
			nlohmann::json block;
			in >> block;
			b.ownerID = block["ownerID"];
			b.ownerName = block["ownerName"];
			b.msg = block["msg"];
			return in;
		}
		// write to output stream in JSON format
		friend std::ostream& operator<<(std::ostream& out, const Block& b)
		{
			out << std::setw(4) << b.GetJSON() << std::endl;
			return out;
		}
		// calculate and update block hash
		void Mine(size_t minerID_in, size_t nonce_in);
		// verify that the current hash of the block is the same as
		// the one returned by GetHash
		bool VerifyIntegrity() const;
		// update prevHash
		// meant for when the block has just been created and not added to a chain
		void SetPrevHash(size_t prevHash_in);

		size_t GetPrevHash() const;
		std::string GetOwnerID() const;
		std::string GetOwnerName() const;
		std::string GetMsg() const;
		size_t GetNonce() const;
		size_t GetHash() const;

	private:
		size_t prevHash = 0;
		std::string ownerID;
		std::string ownerName;
		std::string msg;
		size_t nonce = 0;
		size_t minerID = 0;
		size_t hash = 0;
		time_t timestamp = 0;
	};
}
