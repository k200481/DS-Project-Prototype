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

/*
* 
* Questions
*	How exactly are blocks to be verified?
*	What kinds of puzzles do these processes need to solve?
*	Are we going to use cryptographic hashes?
*	
* 
* Uncertain Ideas:
* 
* To do:
*
* Future Ideas:
*	Replacing the message/response queues with a priority queue
*/

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
		Response(size_t sender_id, std::string res)
			:
			Message(typeid(Response), sender_id),
			res(res)
		{}
		std::string GetResult() const
		{
			return res;
		}
	private:
		std::string res;
	};

	// when possible, these typedefs should be used for one's own sanity
	typedef std::shared_ptr<Message> MsgPtr;
	typedef std::function<std::optional<std::string>(const MsgPtr)> Callable;

private:
	// used to define how a process will handle messages it receives
	// also counts to how many times it is called, the process manager
	// automatically removes messages from the queue once all processes have seen them
	class MessageHandler
	{
	public:
		MessageHandler(std::queue<MsgPtr>& msgLine, std::type_index quit_id);
		// called by a process to handle messages
		std::optional<Callable> Handle(const MsgPtr msg) const;
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
	class Process
	{
		enum class State
		{
			Running,
			Waiting,
			Terminated
		};
	public:
		// no default constructors
		Process(size_t PID, const std::queue<MsgPtr>& incoming_messages, std::mutex& mtx, std::mutex& wMtx,
			Callable sendMessage, const MessageHandler& msgHandler);
		// joins the processes and waits for it to exit
		~Process();
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
		void SaveData(const nlohmann::json& j);
		// get data from the process in json form based on a given predicate
		template <typename Pred>
		nlohmann::json GetData(Pred p)
		{
			nlohmann::json arr = nlohmann::json::array();
			std::ifstream in(filename);

			try
			{
				while (in.peek() != EOF)
				{
					std::string str;
					std::getline(in, str);
					auto obj = nlohmann::json::parse(str);
					if (p(obj))
						arr.push_back(obj);
				}
			}
			catch (const std::exception& e)
			{
				std::cout << e.what() << std::endl;
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
		const MessageHandler& msgHandler;
		// used when calling send messages
		std::mutex& wMtx;
		// used to send messages outside of the thread
		Callable sendMessage;
		// contains messages from the outside (also potentially from othr processes)
		const std::queue<MsgPtr>& incoming_messages;
		// the actual process/minor
		std::thread t;
	};

public:
	/*Basic*/
	// ctor
	ProcessManager(size_t num_processes);
	// dtor
	~ProcessManager();
	// add a handler function to the message handler
	void AddHandlerFunction(std::type_index msg_id, Callable func);
	
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
	
	/*Block Related*/
	// passes given json block to the specified process to store
	void SaveBlock(size_t PID, nlohmann::json j);
	// returs a json array of all blocks that satisfy the given predicate
	template <typename Pred>
	nlohmann::json GetBlocks(Pred pred)
	{
		nlohmann::json arr = nlohmann::json::array();
		for (auto& p : processes)
		{
			nlohmann::json data = p->GetData(pred);
			arr.insert(arr.end(), data.begin(), data.end());
		}
		return arr;
	}
	// broadcasts passed message to all processes, waits for processes to complete processing then 
	// removes all responses from the queue and passes the block to the process that comleted first
	void MineBlock(MsgPtr msg, nlohmann::json& block);

private:
	// this will push a quit message to the queue
	// all threads will terminate once the reach it
	void PostQuitMessage();
	// add a new process to the system
	void AddProcess(size_t id);
	// add multiple processes
	void AddProcesses(size_t num_processes);

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
	std::vector<std::unique_ptr<Process>> processes;
	std::queue<MsgPtr> msgLine;
	std::queue<MsgPtr> processResults;
	MessageHandler msgHandler;
};
