#pragma once
#include <typeindex>
#include <memory>
#include "Block.h"

namespace Blockchain
{
	// the base message class used to communicate within the network
	// it can be inherited from to implement all kinds of functionality
	class Message
	{
	public:
		// parameterized ctor, no defaults
		Message(size_t senderID);
		// virtual dtor as many classes will be inheriting from this class
		virtual ~Message() = default;// { std::cout << ID << ": Im die\n"; };
		// id used to uniquely identify messages
		size_t GetID() const;
		// any ID to differentiate the messages
		// this MUST match with a function id in the message handler
		virtual std::type_index GetTypeID() const = 0;
		// id of the process broadcasting the message (could also be main or the manager itself)
		size_t GetSenderID() const;
	private:
		static size_t count;
		const size_t ID;
		const size_t senderID;
	};

	// by default, shared ptrs are used in the system to allow polymorphism
	// and automated memory management
	typedef std::shared_ptr<Message> MsgPtr;
}
