#include "Base/Memory.hpp"

namespace VkMall
{
    namespace Base
    {
        void* Allocation::Map()
        {
            if(MemType == Enums::eLocal)
                std::cout << "Just tried to call Map() on an allocation on the GPU (Device Local memory). This memory can not be mapped, will instead return nullptr.\n";

            if(pMemory == nullptr)
                pMemory = pAllocator->Map(this);
            
            return pMemory;
        }

        void Allocation::Free()
        {
            pAllocator->Free(this);
        }

        void* Page::Map(size_t Offset)
        {
            if(!bMapped)
            {
                vkMapMemory(*pDevice, Memory, 0, PageSize, 0, &pMemory);
                bMapped = true;
            }
            
            // Cast pMemory to a char(1-byte stride) and move (offset) bytes forward and recast that address as void*
            return (void*)(((char*)pMemory)+Offset);
        }

        void Page::Free(Allocation* pAllocation)
        {
            if(pAllocation == HeadAllocation)
            {
                HeadAllocation = HeadAllocation->pNext;
            }

            Allocation* Iterator = HeadAllocation;

            while(Iterator)
            {
                if(pAllocation == Iterator->pNext)
                {
                    Allocation* tmp = Iterator->pNext->pNext;
                    Iterator->pNext = nullptr;
                    Iterator->pNext = tmp;
                    break;
                }
                
                if(Iterator->pNext != nullptr) Iterator = Iterator->pNext;
                else throw std::runtime_error("Wrappers : Allocator : failed to free allocation, Allocation does not exist in page\n");
            }
        }

        void Allocator::Free(Allocation* pAllocation)
        {
            for(uint32_t i = 0; i < DeviceLocalPages.size(); i++)
            {
                // if the memory handles match, they occupy the same VkDeviceMemory allocation
                if(pAllocation->Memory == DeviceLocalPages[i]->Memory)
                {
                    DeviceLocalPages[i]->Free(pAllocation);
                }
            }
            
            for(uint32_t i = 0; i < HostVisiblePages.size(); i++)
            {
                // if the memory handles match, they occupy the same VkDeviceMemory allocation
                if(pAllocation->Memory == HostVisiblePages[i]->Memory)
                {
                    HostVisiblePages[i]->Free(pAllocation);
                }
            }
        }

        void* Allocator::Map(Allocation* pAlloc)
        {
            if(pAlloc->MemType == Enums::eMemoryType::eLocal)
            {
                for(uint32_t i = 0; i < DeviceLocalPages.size(); i++)
                {
                    if(DeviceLocalPages[i]->Memory == pAlloc->Memory)
                    {
                        return DeviceLocalPages[i]->Map(pAlloc->Offset);
                    }
                }
            }
            else
            {
                for(uint32_t i = 0; i < HostVisiblePages.size(); i++)
                {
                    if(HostVisiblePages[i]->Memory == pAlloc->Memory)
                    {
                        return HostVisiblePages[i]->Map(pAlloc->Offset);
                    }
                }
            }
            
            return nullptr;
        }


    }
}
