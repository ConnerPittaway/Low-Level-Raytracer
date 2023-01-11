#include "Sphere.h"


void* Sphere::operator new(size_t size)
{
    if (Trackers::GetSphereTracker()->GetUsingMemoryPool() == true)
    {
        //std::cout << "Allocating to Sphere Pool " << std::endl;
        return Trackers::GetSphereTracker()->GetMemoryPool()->MemoryAllocation(size);
    }
    else
    {
        return ::operator new(size, Trackers::GetSphereTracker());
    }
}

void Sphere::operator delete(void* pMemory)
{
    if (Trackers::GetSphereTracker()->GetUsingMemoryPool() == true)
    {
        //std::cout << "Deleting to Sphere Pool " << std::endl;
        Trackers::GetSphereTracker()->GetMemoryPool()->MemoryDeallocation(pMemory);
    }
    else
    {
        Header* pHeader = (Header*)((char*)pMemory - sizeof(Header));
        Trackers::GetSphereTracker()->MemoryDeallocation(pHeader); //Seems more accurate
        //pHeader->pTracker->MemoryDeallocation(pHeader);
        free(pHeader);
     }
}
