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

/*
* To do:
*
* Future Ideas:
*	Replacing the message queue with a priority queue
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
		Message(std::type_index typeID, int senderID);
		// virtual dtor as many classes will be inheriting from this class
		virtual ~Message() = default;// { std::cout << ID << ": Im die\n"; };
		// id used to uniquely identify messages
		int GetID() const;
		// any ID to differentiate the messages
		// this MUST match with a function id in the message handler
		std::type_index GetMessageTypeID() const;
		// id of the process broadcasting the message (could also be main or the manager itself)
		int GetSenderID() const;
	private:
		static int count;
		const int ID;
		std::type_index messageTypeID;
		const int senderID;
	};

	// used to define how a process will handle messages it receives
	// also counts to how many times it is called, the process manager
	// automatically removes messages from the queue once all processes have seen them
	class MessageHandler
	{
	public:
		MessageHandler(std::queue<std::shared_ptr<Message>>& msgLine, std::type_index quit_id);
		// called by a process to handle messages
		std::optional<std::function<void(const std::shared_ptr<Message>)>> Handle(const std::shared_ptr<Message> msg) const;
		// MUST update this after adding a new process
		void SetNumProcesses(size_t num_processes);
		void AddFunc(std::type_index id, std::function<void(const std::shared_ptr<Message>)>);
	private:
		size_t num_processes = 0;
		std::unordered_map<std::type_index, std::function<void(const std::shared_ptr<Message>)>> funcMap;
		std::queue<std::shared_ptr<Message>>& msgLine;
		mutable size_t count = 0;
		const std::type_index quit_id;
	};

	class Process
	{
	public:
		Process(int PID, std::queue<std::shared_ptr<Message>>& incoming_messages, std::mutex& mtx, 
			std::function<void(std::shared_ptr<Message>)> sendMessage, const MessageHandler& msgHandler);
		// gets called one when a new thread starts execution
		void operator()();
		int GetPID() const;
	private:
		const int PID;
		std::mutex& mtx;
		std::queue<std::shared_ptr<Message>>& incoming_messages;
		std::function<void(std::shared_ptr<Message>)> sendMessage;
		const MessageHandler& msgHandler;
	};

public:
	ProcessManager(size_t num_processes = 0);
	~ProcessManager();
	// add a new process to the system
	void AddProcess();
	// add multiple processes
	void AddProcesses(size_t num_processes);
	// this will push a quit message to the queue
	// all threads will terminate once the reach it
	void PostQuitMessage();
	// adds a mesage to the queue to make it visible to all processes
	void BroadcastMessage(std::shared_ptr<Message> msg)
	{
		msgLine.push(msg);
	}
	// add a handler function to the message handler
	// try to add all the functions before adding any processes
	// otherwise there MIGHT be some synchronization issues
	void AddHandlerFunction(std::type_index msg_id, std::function<void(const std::shared_ptr<Message>)> func);

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
	std::vector<std::thread> threads;
	std::queue<std::shared_ptr<Message>> msgLine;
	MessageHandler msgHandler;
};
