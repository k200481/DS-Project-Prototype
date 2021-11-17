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
#include <iostream>

/*
* To do:
*	Make the Message Handler a member and make a function to add handler functions to it
*	Replace the messagsID system with one that uses typeids
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
		Message(int typeID, int senderID);
		// virtual dtor as many classes will be inheriting from this class
		virtual ~Message() = default;// { std::cout << ID << ": Im die\n"; };
		// id used to uniquely identify messages
		int GetID() const;
		// any ID to differentiate the messages
		// this MUST match with a function id in the message handler
		int GetMessageTypeID() const;
		// id of the process broadcasting the message (could also be main or the manager itself)
		int GetSenderID() const;
	private:
		static int count;
		const int ID;
		const int messageTypeID;
		const int senderID;
	};

	// used to define how a process will handle messages it receives
	// also counts to how many times it is called, the process manager
	// can use this information to decide if a message should be popped from the queue
	class MessageHandler : 
		public std::unordered_map<int, std::function<void(const std::unique_ptr<Message>&)>>
	{
	public:
		// called by a process to handle messages
		std::optional<std::function<void(const std::unique_ptr<Message>&)>> Handle(const std::unique_ptr<Message>& msg) const;
		// ge tthe number of times the handler has been called sin the last reset
		int GetCount() const;
		// reset the number of messages
		void Reset();

	private:
		mutable int count = 0;
	};

	class Process
	{
	public:
		Process(int PID, std::queue<std::unique_ptr<Message>>& incoming_messages, std::mutex& mtx, 
			std::function<void(std::unique_ptr<Message>)> sendMessage, const MessageHandler& msgHandler);
		// gets called one when a new thread starts execution
		void operator()();
	private:
		const int PID;
		std::mutex& mtx;
		std::queue<std::unique_ptr<Message>>& incoming_messages;
		std::function<void(std::unique_ptr<Message>)> sendMessage;
		const MessageHandler& msgHandler;
	};

public:
	ProcessManager(MessageHandler msgHandler);
	~ProcessManager();
	// add a new process to the system
	void AddProcess();
	// meant to be called after some appropriate intervals
	// if all processes are done processing a message, it will pop it from the queue
	void Update();
	// this will push a quit message to the queue
	// all threads will terminate once the reach it
	void PostQuitMessage();
	// adds a mesage to the queue to make it visible to all processes
	void BroadcastMessage(std::unique_ptr<Message> msg)
	{
		msgLine.push(std::move(msg));
	}

private:
	std::mutex mtx;
	std::vector<std::thread> threads;
	std::queue<std::unique_ptr<Message>> msgLine;
	MessageHandler msgHandler;
};
