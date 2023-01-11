#include "ThreadManager.h"
#include "Trackers.h"
#include <iostream>

static int maxThreads = std::thread::hardware_concurrency(); //Maximum number of supported threads

ThreadManager::ThreadManager()
{
    terminateThreads = false;
    taskNumber = 0;
    threadPool.resize(maxThreads);
    for (int i = 0; i < maxThreads; i++)
    {
        threadPool.at(i) = std::thread(&ThreadManager::RunThreads, this); //Assign infinite run function to threadpool
    }

}

ThreadManager::~ThreadManager()
{
   StopThreads();
}

void ThreadManager::QueueThreadTask(const std::function<void()>& task)
{
#ifdef _DEBUG
    std::cout << "Task Queued " << std::endl;
#endif
    std::unique_lock<std::mutex> lock(mutexThreadPool);
    tasks.push(task);
    lock.unlock();
    //Unblock thread waiting on condition -> runs the task
    mutexConditions.notify_one();
}

void ThreadManager::StopThreads()
{
    terminateThreads = true; //Atomic Bool Thread Safe
    //Notify all threads -> finishes all tasks on all threads
    mutexConditions.notify_all();

    //int i = 0;
    for (std::thread& active_thread : threadPool) 
    {
        active_thread.join(); //Join threads to main
      //  std::cout << "Closing Thread " << i << std::endl;
       // i++;
    }
    threadPool.clear(); //Clear thread pool
}

void ThreadManager::RunThreads()
{
    std::function<void()> taskToRun; //Placeholder for function 
    while (true) 
    {
        {
            std::unique_lock<std::mutex> lock(mutexThreadPool);
            mutexConditions.wait(lock, [this]() //Block condition until task list is not empty
            {
                return terminateThreads || !tasks.empty(); //Return if the tasks list is open or threads have been terminated
            });

            if (terminateThreads && tasks.empty()) //If all tasks completed
            {
                return; //If task is empty and threads have been terminated thread will join
            }
            taskToRun = tasks.front(); //Retrieve task from front of queue
            tasks.pop(); //Remove from front FIFO
            lock.unlock();
        }
        taskToRun(); // Run task
#ifdef _DEBUG
        std::cout << "Running task" << std::endl;
#endif
    }
}

void* ThreadManager::operator new(size_t size)
{
    if (Trackers::GetThreadingTracker()->GetUsingMemoryPool() == true)
    {
        return Trackers::GetThreadingTracker()->GetMemoryPool()->MemoryAllocation(size);
    }
    else
    {
        return ::operator new(size, Trackers::GetThreadingTracker());
    }
}

void ThreadManager::operator delete(void* pMemory)
{
    if (Trackers::GetThreadingTracker()->GetUsingMemoryPool() == true)
    {
        Trackers::GetThreadingTracker()->GetMemoryPool()->MemoryDeallocation(pMemory);
    }
    else
    {
        Header* pHeader = (Header*)((char*)pMemory - sizeof(Header));
        Trackers::GetThreadingTracker()->MemoryDeallocation(pHeader); //Seems more accurate
        //pHeader->pTracker->MemoryDeallocation(pHeader);
        free(pHeader);
    }
}

