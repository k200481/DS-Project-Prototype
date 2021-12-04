#pragma once
#include <Windows.h>
#include <vector>
#include <unordered_map>
#include "nlohmann.h"
#include <string>
#include "Message.h"
#include "Miner.h"

namespace Blockchain
{
	// used to define how a miner will react to messages it receives
	// also counts to how many times it is called and automatically removes 
	// messages from the queue once all miners have seen them
	// doesn't feel right putting it outside of network manager since it is
	// so closely tied to it, but have to for the system to work
	class MessageHandlerMap
	{
	public:
		MessageHandlerMap(std::queue<MsgPtr>& msgLine, std::type_index quit_id);
		// returns a function depending on the message typeID
		std::optional<Callable> GetMessageHandler(const MsgPtr msg) const;
		// MUST update this after adding a new process
		void SetNumMiners(size_t numMiners);
		void AddFunc(std::type_index id, Callable);
	private:
		size_t numMiners = 0;
		std::unordered_map<std::type_index, Callable> funcMap;
		std::queue<MsgPtr>& msgLine;
		mutable size_t count = 0;
		const std::type_index quit_id;
	};

	class NetworkManager
	{
	public:
		/*Basic*/
		// ctor
		NetworkManager(size_t num_processes);
		// dtor
		~NetworkManager();
		// add a handler function to the message handler
		void AddMessageHandler(std::type_index msg_id, Callable func);

		/*Block Related*/
		// passes given json block to the specified process to store
		void SaveBlock(size_t PID, nlohmann::json j);
		// returs a json array of all blocks that satisfy the given predicate
		template <typename Pred>
		nlohmann::json GetBlocks(Pred pred)
		{
			nlohmann::json arr = nlohmann::json::array();
			for (auto& p : miners)
			{
				nlohmann::json data = p->GetBlocks(pred);
				arr.insert(arr.end(), data.begin(), data.end());
			}
			return arr;
		}
		// broadcasts passed message to all processes, waits for processes to complete processing then 
		// removes all responses from the queue and passes the block to the process that comleted first
		// returns true if mining was successful, false otherwise
		bool MineBlock(MsgPtr msg);

	private:
		// this will push a quit message to the queue
		// all threads will terminate once the reach it
		void PostQuitMessage();
		// add a new process to the system
		void AddProcess(size_t id);
		// add multiple processes
		void AddProcesses(size_t num_processes);

		/*Processing Management Related*/
		// adds a mesage to the queue to make it visible to all processes
		void BroadcastMessage(MsgPtr msg);
		// check if processes are running or if messages are left to be processed
		bool Completed() const;
		// waits until Completed() returns true
		void WaitForCompletion() const;
		// check if the response queue has any responses in it
		bool ResponsesAreAvailable() const;
		// removes the first recieved response from the response queue and returns it
		MsgPtr GetFirstResponse();

	private:
		class QuitMessage : public Message
		{
		public:
			QuitMessage()
				:
				Message(0)
			{}
			virtual std::type_index GetTypeID() const override
			{
				return typeid(QuitMessage);
			}
		};

	private:
		std::mutex mtx;
		std::mutex wMtx;
		std::vector<std::unique_ptr<class Miner>> miners;
		std::queue<MsgPtr> msgLine;
		std::queue<MsgPtr> processResults;
		MessageHandlerMap msgHandler;
	};
}


