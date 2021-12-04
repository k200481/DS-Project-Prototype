#include "Block.h"
#include <iostream>

using json = nlohmann::json;
using namespace std::chrono;

json Blockchain::Block::GetJSON() const
{
	json block;
	block["prevHash"] = prevHash;
	block["ownerID"] = ownerID;
	block["ownerName"] = ownerName;
	block["msg"] = msg;
	block["nonce"] = nonce;
	block["minerID"] = minerID;
	block["hash"] = hash;
	char temp[255];
	ctime_s(temp, 255, &timestamp);
	temp[strlen(temp) - 1] = '\0';
	block["timestamp"] = temp;
	return block;
}

void Blockchain::Block::Mine(size_t minerID_in, size_t nonce_in)
{
	this->minerID = minerID_in;
	this->nonce = nonce_in;
	std::ostringstream oss;
	oss << prevHash << ownerID << ownerName << msg << nonce << minerID;
	std::hash<std::string> hasher;
	hash = hasher(oss.str());
	timestamp = system_clock::to_time_t(system_clock::now());
}

bool Blockchain::Block::VerifyIntegrity() const
{
	std::ostringstream oss;
	oss << prevHash << ownerID << ownerName << msg << nonce << minerID;
	std::hash<std::string> hasher;
	auto newHash = hasher(oss.str());
	return hash == newHash;
}


void Blockchain::Block::SetPrevHash(size_t prevHash_in)
{
	prevHash = prevHash_in;
}

size_t Blockchain::Block::GetPrevHash() const
{
	return prevHash;
}

std::string Blockchain::Block::GetOwnerID() const
{
	return ownerID;
}

std::string Blockchain::Block::GetOwnerName() const
{
	return ownerName;
}

std::string Blockchain::Block::GetMsg() const
{
	return msg;
}

size_t Blockchain::Block::GetNonce() const
{
	return nonce;
}

size_t Blockchain::Block::GetHash() const
{
	return hash;
}
