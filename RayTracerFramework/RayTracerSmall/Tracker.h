#pragma once
#include <iostream>
#include "MemoryPool.h"
#include "GlobalNewDelete.h"

class Tracker
{
public:
	Tracker();
	void Walk();
	void MemoryAllocation(Header* header);
	void MemoryDeallocation(Header* header);
	void SetMemoryPool(MemoryPool* pMemPool);
	void SetUsingMemoryPool(bool isUsingMemoryPool);

	MemoryPool* GetMemoryPool()
	{
		return pMemoryPool;
	}

	bool GetUsingMemoryPool()
	{
		return usingMemoryPool;
	}

private:
	int trackerSize = 0;
	int maximumSize = 0;
	Header* head = nullptr;

	bool usingMemoryPool = false;
	MemoryPool* pMemoryPool = nullptr;
};

