#include "Block.h"
#include <iostream>

nlohmann::json Block::GetJSON() const
{
	nlohmann::json block;
	block["ownerID"] = ownerID;
	block["ownerName"] = ownerName;
	block["msg"] = msg;
	block["hash"] = hash;
	block["nonce"] = nonce;
	char temp[255];
	ctime_s(temp, 255, &timestamp);
	temp[strlen(temp) - 1] = '\0';
	block["timestamp"] = temp;
	return block;
}
