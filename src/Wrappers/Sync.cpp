#include "Wrappers/Sync.hpp"

namespace VkMall
{
    namespace Wrappers
    {
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

        CommandSync::CommandSync()
        {
            SignalFence = nullptr;
            SignalList = {};
            WaitList = {};
        }

        bool CommandSync::Signal(Fence* pFence)
        {
            if(SignalFence == nullptr)
            {
                SignalFence = pFence;
                return true;
            }
            else
            {
                return false;
            }
        }

        bool CommandSync::Signal(Semaphore* pSemaphore)
        {
            if(pSemaphore != nullptr)
                SignalList.push_back(pSemaphore);
        }

        bool CommandSync::Wait(Semaphore* pSemaphore)
        {
            if(pSemaphore != nullptr)
                WaitList.push_back(pSemaphore);
        }
    }
}