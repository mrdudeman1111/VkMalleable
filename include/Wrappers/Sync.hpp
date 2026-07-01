#pragma once

#include "Base/VulkanObject.hpp"

namespace VkMall
{
    namespace Wrappers
    {
        class Fence : VulkanObject
        {
            public:
            Fence(VkDevice* Device, bool bSignaled = false, std::string FenceName = "Fence");
            ~Fence();

            operator VkFence()
            {
                return vkFence;
            }

            operator VkFence*()
            {
                return &vkFence;
            }

            void Wait();

            private:
            VkFence vkFence;
        };

        class Semaphore : VulkanObject
        {
            public:
            Semaphore(VkDevice* Device, std::string SemaphoreName = "Semaphore");
            ~Semaphore();

            operator VkSemaphore()
            {
                return vkSemaphore;
            }

            operator VkSemaphore*()
            {
                return &vkSemaphore;
            }

            private:
            VkSemaphore vkSemaphore;
        };

        /*! \brief Wraps command submition synchronization. For future use. */
        struct CommandSync
        {
        public:
            CommandSync();

            bool Signal(Fence* pFence);
            bool Signal(Semaphore* pSemaphore);
            bool Wait(Semaphore* pSemaphore);

        private:
            Fence* SignalFence;
            std::vector<Semaphore*> SignalList;
            std::vector<Semaphore*> WaitList;
        };
    }
}
