#pragma once
#include "nlohmann.h"

class Block
{
public:
	Block() = default;
	Block(std::string ownerID, std::string ownerName, std::string msg)
		:
		ownerID(ownerID),
		ownerName(ownerName),
		msg(msg)
	{}
	// returns a JSON object constructed from the block
	nlohmann::json GetJSON() const;
	// reads a json input stream into the block
	void ReadFromJSON(std::istream& in)
	{
		nlohmann::json block;
		in >> block;
		ownerID = block["ownerID"];
		ownerName = block["ownerName"];
		msg = block["msg"];
	}
	// read regular input from an inpu stream
	friend std::istream& operator>>(std::istream& in, Block& b)
	{
		in >> b.ownerID >> b.ownerName >> b.msg;
		return in;
	}
	// print block in json format
	friend std::ostream& operator<<(std::ostream& out, const Block& b)
	{
		out << std::setw(4) << b.GetJSON() << std::endl;
		return out;
	}

	std::string GetOwnerID() const
	{
		return ownerID;
	}
	std::string GetOwnerName() const
	{
		return ownerName;
	}
	std::string GetMsg() const
	{
		return msg;
	}
	size_t GetHash() const
	{
		return hash;
	}
	size_t GetNonce() const
	{
		return nonce;
	}

	void UpdateMiningInfo(size_t res_hash, size_t res_nonce)
	{
		using namespace std::chrono;

		hash = res_hash;
		nonce = res_nonce;
		timestamp = system_clock::to_time_t(system_clock::now());
	}

private:
	std::string ownerID;
	std::string ownerName;
	std::string msg;
	size_t hash = 0;
	size_t nonce = 0;
	time_t timestamp = 0;
};
