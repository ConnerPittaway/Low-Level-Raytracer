#include "Tracker.h"

Tracker::Tracker()
{
}

void Tracker::Walk()
{
	if (head != nullptr)
	{
		Header* currentMemory = head;
		int currentIndex = 0;

		std::cout << "Header Memory Assigned At Position: " << currentIndex << " in Linked List at Memory Address " << &currentMemory << std::endl;
		while (currentMemory->pNext != nullptr)
		{
			currentIndex++;
			std::cout << "Memory Assigned At Position: " << currentIndex << " in Linked List at Memory Address: " << &currentMemory->pNext << std::endl;
			currentMemory = currentMemory->pNext;
		}
		std::cout << "Memory Tracker Finished Walking" << std::endl;
	}
	else
	{
		std::cout << "Memory Tracker Empty" << std::endl;
	}
	std::cout << "Maximum Memory Used: " << maximumSize << " bytes" << std::endl
			  << "Current Memory Used: " << trackerSize << " bytes" << std::endl;
}

void Tracker::MemoryAllocation(Header* header)
{
	trackerSize += header->size;

	if (trackerSize > maximumSize)
	{
		maximumSize = trackerSize;
	}

	//Doubly Linked List
	header->pPrevious = nullptr;
	header->pNext = head;
	if (head)
	{
		head->pPrevious = header;
	}
	head = header;
}

void Tracker::MemoryDeallocation(Header* header)
{
	trackerSize -= header->size; //remove the memory stored in size from the allocated size
	//Doubly Linked List
	if (head == header) //If node is the head node
	{
		head = header->pNext;
	}
	if (header->pNext != nullptr) //If node is not the last node
	{
		header->pNext->pPrevious = header->pPrevious;
	}
	if (header->pPrevious != nullptr) //If node is not the first node
	{
		header->pPrevious->pNext = header->pNext;
	}
}

void Tracker::SetMemoryPool(MemoryPool* pMemPool)
{
	pMemoryPool = pMemPool;
}

void Tracker::SetUsingMemoryPool(bool isUsingMemoryPool)
{
	usingMemoryPool = isUsingMemoryPool;
}
