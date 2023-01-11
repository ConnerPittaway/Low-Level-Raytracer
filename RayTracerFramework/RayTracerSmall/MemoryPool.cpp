#include "MemoryPool.h"
#include "Trackers.h"

MemoryPool::MemoryPool(unsigned long blockNum, unsigned long blockSizeinBytes)
{
	//Calculate Size of Memory Pool based on block size and number
	blockSize = blockSizeinBytes;
	memoryPoolSize = (blockNum * (blockSizeinBytes + sizeof(memoryNode)));

	//Assign memory pool to memory with the size required
	pMemoryPool = (char*)malloc(memoryPoolSize);

	//If successfully allocated
	if (pMemoryPool)
	{
		for (unsigned int i = 0; i < blockNum; i++)
		{
			//Create Memory Node for linked list
			memoryNode* pMemoryNode = (memoryNode*)((char*)pMemoryPool + i * (blockSizeinBytes + sizeof(memoryNode)));

			//Assign to free memory list
			pMemoryNode->pNext = pFreeMemory;
			pMemoryNode->pPrevious = nullptr;

			if (pFreeMemory != nullptr)
			{
				pFreeMemory->pPrevious = pMemoryNode;
			}

			//Set new head of free memory
			pFreeMemory = pMemoryNode;
		}
	}
	else
	{
		std::cout << "Memory Pool Intialisation Failed" << std::endl;
	}
}

MemoryPool::~MemoryPool()
{
	//Delete Memory Pool from Memory
	free(pMemoryPool);
}

void* MemoryPool::MemoryAllocation(unsigned long blockSizeToAllocate)
{
	if (blockSizeToAllocate > blockSize)
	{
#ifdef DEBUG
		std::cout << "Block Size larger than memory pool allocating as standard" << std::endl;
#endif 
		return (char*)malloc(blockSizeToAllocate); //Return Standard Allocation
	}
	else if (pFreeMemory == nullptr)
	{
#ifdef DEBUG
		std::cout << "No free memory allocating as standard" << std::endl;
#endif 
		return (char*)malloc(blockSizeToAllocate); //Return Standard Allocation
	}

	memoryMutex.lock();
	//Retrieve free memory and remove from free memory
	memoryNode* pMemoryNode = pFreeMemory;
	pFreeMemory = pMemoryNode->pNext;

	if (pFreeMemory != nullptr)
	{
		pFreeMemory->pPrevious = nullptr;
	}

	//Assign to allocated memory
	pMemoryNode->pNext = pAllocatedMemory;
	if (pAllocatedMemory != nullptr)
	{
		pAllocatedMemory->pPrevious = pMemoryNode;
	}

	//Set new head of allocated memory
	pAllocatedMemory = pMemoryNode;
	memoryMutex.unlock();
	//Return the memory allocation 
	return (void*)((char*)pMemoryNode + sizeof(memoryNode));
}

void MemoryPool::MemoryDeallocation(void* pMem)
{
	//If memory to free is in the pool
	if (pMemoryPool <= pMem && pMem < ((char*)pMemoryPool + memoryPoolSize))
	{
		memoryMutex.lock();
		//Assign Node to the current memory to delete
		memoryNode* pMemoryNode = (memoryNode*)((char*)pMem - sizeof(memoryNode));

		//Retrieve values from node
		memoryNode* pNext = pMemoryNode->pNext;
		memoryNode* pPrevious = pMemoryNode->pPrevious;

		//Remove from allocated list
		if (pPrevious == nullptr)
		{
			pAllocatedMemory = pMemoryNode->pNext;
		}
		else 
		{
			pPrevious->pNext = pNext;
		}
		if (pNext != nullptr)
		{
			pNext->pPrevious = pPrevious;
		}

		//Add to free memory list
		pMemoryNode->pNext = pFreeMemory;
		if (pFreeMemory != nullptr)
		{
			pFreeMemory->pPrevious = pMemoryNode;
		}

		//Set new head of free memory
		pFreeMemory = pMemoryNode;
		memoryMutex.unlock();
	}
	else
	{
#ifdef DEBUG
		std::cout << "Memory not in pool releasing with free" << std::endl;
#endif 
		free(pMem); //Use free as memory is not in pool
	}
}


//Assigners to Tracker Class
void* MemoryPool::operator new(size_t size)
{
	if (Trackers::GetMemoryPoolTracker()->GetUsingMemoryPool() == true)
	{
		return Trackers::GetMemoryPoolTracker()->GetMemoryPool()->MemoryAllocation(size);
	}
	else
	{
		return ::operator new(size, Trackers::GetMemoryPoolTracker());
	}
}

void MemoryPool::operator delete(void* pMemory)
{
	if (Trackers::GetMemoryPoolTracker()->GetUsingMemoryPool() == true)
	{
		Trackers::GetMemoryPoolTracker()->GetMemoryPool()->MemoryDeallocation(pMemory);
	}
	else
	{
		Header* pHeader = (Header*)((char*)pMemory - sizeof(Header));
		Trackers::GetMemoryPoolTracker()->MemoryDeallocation(pHeader); //Seems more accurate
		//pHeader->pTracker->MemoryDeallocation(pHeader);
		free(pHeader);
	}
}
