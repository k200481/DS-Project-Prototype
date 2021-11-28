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
* 
* Uncertain Ideas:
*	Making a "response" class for processes to return their responses if there are any
*	Make a func or repurpose "SendMsg" to add responses to the response queue
*	Give processes direct write-access to the response queue
*	Making thread a member of process instead of templating threads on processes
*		comes with far too many benifits why didn't I do this from the start T^T
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

	// when possible, these typedefs should be used for one's own sanity
	typedef std::shared_ptr<Message> MsgPtr;
	typedef std::function<std::optional<std::string>(const MsgPtr)> Callable;

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

	class Process
	{
	public:
		Process(size_t PID, const std::queue<MsgPtr>& incoming_messages, std::mutex& mtx, std::mutex& wMtx,
			Callable sendMessage, const MessageHandler& msgHandler);
		// gets called one when a new thread starts execution
		void operator()();
		size_t GetPID() const;
	private:
		static size_t count;
		const size_t PID;
		std::mutex& mtx;
		std::mutex& wMtx;
		const std::queue<MsgPtr>& incoming_messages;
		Callable sendMessage;
		const MessageHandler& msgHandler;
	};

public:
	ProcessManager(std::queue<std::string>& output_queue, size_t num_processes = 0);
	~ProcessManager();
	// add a new process to the system
	void AddProcess();
	// add multiple processes
	void AddProcesses(size_t num_processes);
	// this will push a quit message to the queue
	// all threads will terminate once the reach it
	void PostQuitMessage();
	// adds a mesage to the queue to make it visible to all processes
	void BroadcastMessage(MsgPtr msg)
	{
		msgLine.push(msg);
	}
	// add a handler function to the message handler
	// try to add all the functions before adding any processes
	// otherwise there MIGHT be some synchronization issues
	void AddHandlerFunction(std::type_index msg_id, Callable func);
	// check if there are any messages left to be processed
	bool Completed() const
	{
		return msgLine.empty();
	}

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
	std::vector<std::thread> threads;
	std::queue<MsgPtr> msgLine;
	std::queue<std::string> processResults;
	MessageHandler msgHandler;
};
