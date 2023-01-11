#pragma once
#include <thread>
#include <functional>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <stdlib.h>

class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	void QueueThreadTask(const std::function<void()>& task);
	void StopThreads();

	void* operator new(size_t size);
	void operator delete(void* pMemory);
private:
	int taskNumber;
	void RunThreads();
	std::queue<std::function<void()>> tasks; //Store to run functions FIFO
	std::atomic<bool> terminateThreads; //Use atomic bool to avoid race condition
	std::condition_variable mutexConditions; //Blocks threads on condition 
	std::vector<std::thread> threadPool; //Store threads
	std::mutex mutexThreadPool; //Mutex for thread safety in pool
};
