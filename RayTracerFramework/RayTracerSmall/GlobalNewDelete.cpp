#include "GlobalNewDelete.h"
#include "Tracker.h"
#include "Trackers.h"

void* operator new(size_t size, Tracker* pTracker)
{
	if (pTracker->GetUsingMemoryPool() == true)
	{
		return pTracker->GetMemoryPool()->MemoryAllocation(size);
	}
	else
	{
		size_t numberOfBytes = size + sizeof(Header) + sizeof(Footer);
		char* pMemory = (char*)malloc(numberOfBytes);
		Header* pHeader = (Header*)pMemory;

		pHeader->pTracker = pTracker;
		pHeader->size = size;
		pHeader->checkValue = 0xDEADC0DE;

		pHeader->pTracker->MemoryAllocation(pHeader);

		void* pFooterStart = pMemory + sizeof(Header) + size;
		Footer* pFooter = (Footer*)pFooterStart;
		pFooter->checkValue = 0xDEADBEEF;

		void* startOfMemoryBlock = pMemory + sizeof(Header);
		return startOfMemoryBlock;
	}
}

void* operator new(size_t size)
{
	return ::operator new(size, Trackers::GetDefaultTracker());
}

void operator delete(void* pMemory)
{
	if (Trackers::GetDefaultTracker()->GetUsingMemoryPool() == true)
	{
		Trackers::GetDefaultTracker()->GetMemoryPool()->MemoryDeallocation(pMemory);
	}
	else
	{
		Header* pHeader = (Header*)((char*)pMemory - sizeof(Header));
		if (pHeader->checkValue != 0xDEADC0DE)
		{
			std::cout << "Failure within header" << std::endl;
		}

		Footer* pFooter = (Footer*)((char*)pMemory + pHeader->size);
		if (pFooter->checkValue != 0xDEADBEEF)
		{
			std::cout << "Failure within footer" << std::endl;
		}

		pHeader->pTracker->MemoryDeallocation(pHeader);
		//Trackers::GetDefaultTracker()->MemoryDeallocation(pHeader);
		free(pHeader);
	}
}
