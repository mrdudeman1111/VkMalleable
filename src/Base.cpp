#include "Context.hpp"
#include <vulkan/vulkan_core.h>

namespace Wrappers
{
namespace Memory
{
void* Allocation::Map()
{
    if(pMemory == nullptr)
    {
        pMemory = pAllocator->Map(this);
    }
    
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

Fence::Fence(VkDevice* Device, bool bSignaled, std::string FenceName) : VulkanObject(Device, FenceName)
{
    VkFenceCreateInfo FenceCreateInfo{};
    FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceCreateInfo.flags = (bSignaled) ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    
    if(vkCreateFence(*Device, &FenceCreateInfo, nullptr, &vkFence) != VK_SUCCESS)
    {
        throw std::runtime_error("Wrappers : failed to create fence\n");
    }
}

Fence::~Fence()
{
    vkDestroyFence(*pDevice, vkFence, nullptr);
}

void Fence::Wait()
{
    vkWaitForFences(*pDevice, 1, &vkFence, VK_TRUE, UINT64_MAX);
    vkResetFences(*pDevice, 1, &vkFence);
}

Semaphore::Semaphore(VkDevice* Device, std::string SemaphoreName) : VulkanObject(Device, SemaphoreName)
{
    VkSemaphoreCreateInfo SemCI{};
    SemCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    if(vkCreateSemaphore(*pDevice, &SemCI, nullptr, &vkSemaphore) != VK_SUCCESS)
    {
        throw std::runtime_error("Wrappers : failed to create Semaphore\n");
    }
}

Semaphore::~Semaphore()
{
    vkDestroySemaphore(*pDevice, vkSemaphore, nullptr);
}

CommandBuffer::CommandBuffer(VkDevice& Device, VkCommandPool* AllocatorPool, VkQueue* pQueue, VkCommandBuffer pCmdBuffer, Enums::CommandType cmdType) : pPool(AllocatorPool), cmdQueue(pQueue), vkBuffer(pCmdBuffer), Type(cmdType), VulkanObject(&Device, "Command Buffer")
{
    pSignalFence = nullptr;
    SignalSem = {};
    WaitSem = {};
}

CommandBuffer::~CommandBuffer()
{
    vkFreeCommandBuffers(*pDevice, *pPool, 1, &vkBuffer);
}

void CommandBuffer::Reset()
{
    WaitSem.clear();
    SignalSem.clear();
    pSignalFence = nullptr;
    
    vkResetCommandBuffer(vkBuffer, 0);
}

void CommandBuffer::Begin()
{
    VkCommandBufferBeginInfo BeginInfo{};
    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    vkBeginCommandBuffer(vkBuffer, &BeginInfo);
    
    bRecording = true;
}

void CommandBuffer::End()
{
    vkEndCommandBuffer(vkBuffer);
    
    bRecording = false;
}

bool CommandBuffer::isRecording()
{
    return bRecording;
}

void CommandBuffer::Submit()
{
    std::vector<VkSemaphore> vkWaitSem;
    std::vector<VkSemaphore> vkSignalSem;
    
    for(uint32_t i = 0; i < WaitSem.size(); i++)
    {
        vkWaitSem.push_back(*WaitSem[i]);
    }
    
    for(uint32_t i = 0; i < SignalSem.size(); i++)
    {
        vkSignalSem.push_back(*SignalSem[i]);
    }
    
    VkSubmitInfo SubmitInfo{};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &vkBuffer;
    SubmitInfo.waitSemaphoreCount = (uint32_t)vkWaitSem.size();
    SubmitInfo.pWaitSemaphores = vkWaitSem.data();
    SubmitInfo.signalSemaphoreCount = (uint32_t)vkSignalSem.size();
    SubmitInfo.pSignalSemaphores = vkSignalSem.data();
    //SubmitInfo.pWaitDstStageMask = (PipeStage == VK_PIPELINE_STAGE_NONE) ? nullptr : &PipeStage;
    
    if(pSignalFence == nullptr) vkQueueSubmit(*cmdQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
    else vkQueueSubmit(*cmdQueue, 1, &SubmitInfo, *pSignalFence);
}

void CommandBuffer::Signal(Fence* Fence)
{
    pSignalFence = Fence;
}

void CommandBuffer::Signal(Semaphore* Semaphore)
{
    SignalSem.push_back(Semaphore);
}

void CommandBuffer::Wait(Semaphore* Semaphore)
{
    WaitSem.push_back(Semaphore);
}

}
