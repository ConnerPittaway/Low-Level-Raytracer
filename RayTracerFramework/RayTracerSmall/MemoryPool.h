#pragma once
#include <iostream>
#include <mutex>
class MemoryPool
{
public:
	MemoryPool(unsigned long blockNum, unsigned long blockSizeinBytes);
	~MemoryPool();

	void* MemoryAllocation(unsigned long blockSizeToAllocate);
	void MemoryDeallocation(void* pMem);

	void* operator new(size_t size);
	void operator delete(void* pMemory);
private:
	void* pMemoryPool = nullptr;
	unsigned long blockSize;
	unsigned long memoryPoolSize;

	struct memoryNode
	{
		//Double Linked List -> Similar to header
		memoryNode* pNext = nullptr;
		memoryNode* pPrevious = nullptr;
	};

	//Memory Lists
	memoryNode* pAllocatedMemory = nullptr;
	memoryNode* pFreeMemory = nullptr;

	//Ensure thread safety in allocation and deallocation
	std::mutex memoryMutex;
};


