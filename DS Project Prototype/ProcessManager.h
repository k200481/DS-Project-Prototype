#pragma once
#include <queue>
#include <thread>
#include <Windows.h>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <optional>
#include <typeindex>
#include <fstream>
#include "nlohmann.h"
#include <string>
#include <iostream>
#include "Block.h"

class ProcessManager
{
public:
	// messages used to communicate with the processes
	// this is the base class, main wil inherit from this
	// to create different kinds of messages
	class Message
	{
	public:
		// parameterized ctor, no defaults
		Message(std::type_index typeID, size_t senderID);
		// virtual dtor as many classes will be inheriting from this class
		virtual ~Message() = default;// { std::cout << ID << ": Im die\n"; };
		// id used to uniquely identify messages
		size_t GetID() const;
		// any ID to differentiate the messages
		// this MUST match with a function id in the message handler
		std::type_index GetMessageTypeID() const;
		// id of the process broadcasting the message (could also be main or the manager itself)
		size_t GetSenderID() const;
	private:
		static size_t count;
		const size_t ID;
		std::type_index messageTypeID;
		const size_t senderID;
	};

	// processes use this to send the results of their processing
	class Response : public Message
	{
	public:
		Response(size_t sender_id, const Block& res)
			:
			Message(typeid(Response), sender_id),
			res(res)
		{}
		Block GetBlock() const
		{
			return res;
		}
	private:
		Block res;
	};

	// when possible, these typedefs should be used for one's own sanity
	typedef std::shared_ptr<Message> MsgPtr;
	typedef std::function<std::optional<Block>(const MsgPtr)> Callable;

private:
	// used to define how a process will handle messages it receives
	// also counts to how many times it is called, the process manager
	// automatically removes messages from the queue once all processes have seen them
	class MessageHandlerMap
	{
	public:
		MessageHandlerMap(std::queue<MsgPtr>& msgLine, std::type_index quit_id);
		// called by a process to handle messages
		std::optional<Callable> GetMessageHandler(const MsgPtr msg) const;
		// MUST update this after adding a new process
		void SetNumProcesses(size_t num_processes);
		void AddFunc(std::type_index id, Callable);
	private:
		size_t num_processes = 0;
		std::unordered_map<std::type_index, Callable> funcMap;
		std::queue<MsgPtr>& msgLine;
		mutable size_t count = 0;
		const std::type_index quit_id;
	};

	// holds the miners and data for them to use
	class Miner
	{
		enum class State
		{
			Running,
			Waiting,
			Terminated
		};
	public:
		// no default constructors
		Miner(size_t PID, const std::queue<MsgPtr>& incoming_messages, std::mutex& mtx, std::mutex& wMtx,
			std::function<std::optional<std::string>(const MsgPtr)> sendMessage, const MessageHandlerMap& msgHandler);
		// joins the processes and waits for it to exit
		~Miner();
		size_t GetPID() const;
		
		// check the miner's current state
		bool Waiting() const 
		{ 
			return s == State::Waiting;
		}
		bool Terminated() const
		{
			return s == State::Terminated;
		}
		bool Running() const
		{
			return s == State::Running;
		}
		// save a pased json object
		void SaveBlock(const nlohmann::json& j);
		// get data from the process in json form based on a given predicate
		template <typename Pred>
		nlohmann::json GetBlocks(Pred p)
		{
			nlohmann::json arr = nlohmann::json::array();
			std::ifstream in(filename);

			try
			{
				while (in.peek() != EOF)
				{
					nlohmann::json obj;
					in >> obj;
					if (!obj.is_null() && p(obj))
						arr.push_back(obj);
				}
			}
			catch (const std::exception& e)
			{
				std::cout << PID << ": " << e.what() << std::endl;
			}

			return arr;
		}

	private:
		// gets called once when a new thread starts execution
		void func();
	private:
		std::string filename;

		State s = State::Waiting;
		// thread id
		const size_t PID;
		// everything used by the thread
		// used when calling the message handler
		std::mutex& mtx;
		// used for getting the appropriate function to handle a message
		const MessageHandlerMap& msgHandler;
		// used when calling send messages
		std::mutex& wMtx;
		// used to send messages outside of the thread
		std::function<std::optional<std::string>(const MsgPtr)> sendMessage;
		// contains messages from the outside (also potentially from othr processes)
		const std::queue<MsgPtr>& incoming_messages;
		// the actual process/miner
		std::thread t;
	};

public:
	/*Basic*/
	// ctor
	ProcessManager(size_t num_processes);
	// dtor
	~ProcessManager();
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
	// returns the hash of the mined block
	size_t MineBlock(MsgPtr msg);

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
			Message(typeid(QuitMessage), 0)
		{}
	};

private:
	std::mutex mtx;
	std::mutex wMtx;
	std::vector<std::unique_ptr<Miner>> miners;
	std::queue<MsgPtr> msgLine;
	std::queue<MsgPtr> processResults;
	MessageHandlerMap msgHandler;
};
