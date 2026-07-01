#include "Wrappers/Commands.hpp"

namespace VkMall
{
    namespace Wrappers
    {
        CommandPool::CommandPool(VkDevice& Device, VkQueue* pQueue, uint32_t QueueFamily) : cmdQueue(pQueue), VulkanObject(&Device, "CommandPool")
        {
            VkCommandPoolCreateInfo PoolCI{};
            PoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            PoolCI.queueFamilyIndex = QueueFamily;
            PoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            
            vkCreateCommandPool(Device, &PoolCI, nullptr, &vkPool);
        }

        CommandPool::~CommandPool()
        {
            vkDestroyCommandPool(*pDevice, vkPool, nullptr);
        }

        CommandBuffer* CommandPool::CreateBuffer(Enums::CommandLevel cmdLevel)
        {
            VkCommandBuffer tmp;
            
            VkCommandBufferAllocateInfo AllocInfo{};
            AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            AllocInfo.commandPool = vkPool;
            AllocInfo.commandBufferCount = 1;
            AllocInfo.level = (cmdLevel == Enums::ePrimary) ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
            
            vkAllocateCommandBuffers(*pDevice, &AllocInfo, &tmp);
            
            return new CommandBuffer(*pDevice, &vkPool, cmdQueue, tmp, Type);
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
            // SubmitInfo.pWaitDstStageMask = (PipeStage == VK_PIPELINE_STAGE_NONE) ? nullptr : &PipeStage;
            
            if(pSignalFence == nullptr)
                vkQueueSubmit(*cmdQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
            else
                vkQueueSubmit(*cmdQueue, 1, &SubmitInfo, *pSignalFence);
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
}