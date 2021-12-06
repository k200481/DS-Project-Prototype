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
		template <typename MsgT>
		bool MineBlock(const Block& block);

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
		template <typename MsgT, typename DataT>
		void BroadcastMessage(DataT data);
		template <typename MsgT>
		void BroadcastMessage();
		// check if processes are running or if messages are left to be processed
		bool Completed() const;
		// waits until Completed() returns true
		void WaitForCompletion() const;
		// check if the response queue has any responses in it
		bool ResponsesAreAvailable() const;
		// removes the first recieved response from the response queue and returns it
		MsgPtr GetFirstResponse();
		// miners use this to get messages from the message queue
		std::optional<MsgPtr> MessageHandler(size_t PID);

	private:
		std::mutex mtx;
		std::mutex wMtx;
		std::vector<std::unique_ptr<class Miner>> miners;
		std::queue<MsgPtr> msgLine;
		std::queue<MsgPtr> solutions;
		std::unordered_map<std::type_index, Callable> msgHandlerMap;
		
		size_t readCount = 0;
		std::vector<bool> msgReadBy;
	};
}


