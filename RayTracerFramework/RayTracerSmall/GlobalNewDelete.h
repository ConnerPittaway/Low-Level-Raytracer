#pragma once
#include <iostream>

class Tracker;

struct Header
{
	int size = 0;
	int checkValue;
	Tracker* pTracker;
	Header* pNext = nullptr;
	Header* pPrevious = nullptr;
};

struct Footer
{
	int checkValue;
};

//Overrides
void* operator new(size_t size, Tracker* pTracker);
void operator delete(void* pMemory);

//Default if no tracker is passed
void* operator new(size_t size);
