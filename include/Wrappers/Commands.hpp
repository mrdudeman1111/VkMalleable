#pragma once

#include "Wrappers/Sync.hpp"
#include "Base/Enums.hpp"

namespace VkMall
{
    namespace Wrappers
    {
        /*! \brief A wrapper class for command buffers. These should only be created by a command pool object via CreateBuffer().
        */
        class CommandBuffer : VulkanObject
        {
            public:
            CommandBuffer(VkDevice& Device, VkCommandPool* AllocaterPool, VkQueue* pQueue, VkCommandBuffer pCmdBuffer, Enums::CommandType Type);
            ~CommandBuffer();

            /*! \brief Resets the command buffer so it can be re-recorded. */
            void Reset();

            /*! \brief Begins recording on the command buffer. */
            void Begin();

            /*! \brief ends recording on the command buffer. */
            void End();

            /*! \brief Submits the command buffer to the command pool used by the parent command pool (If the command pool this was allocated from exists on the compute queue, it will be submited to that compute queue) */
            void Submit();

            /*! \brief Adds a fence to the signal list of this command buffer. (We can only signal ONE fence per command buffer)
            *
            * When Submit() is called, this fence is added to the signal list. Meaning when the command buffer finishes executing on the GPU, this fence will be signaled. Best for GPU-CPU synchronization.
            *
            * @param Fence A pointer to the fence object to signal on completion.
            */
            void Signal(Fence* Fence);

            /*! \brief Adds a semaphore to the signal list of this command buffer.
            *
            * When Submit() is called, this semaphore is added to the signal list. Meaning when the command buffer finishes executing on the GPU, this semaphore will be signaled. Best for GPU-GPU synchronization
            *
            * @param Semaphore A pointer to the semaphore object to signal on completion.
            */
            void Signal(Semaphore* Semaphore);

            /*! \brief Adds a semaphore to the wait list of this command buffer.
            *
            * When Submit () is called, we wait for this semaphore to be signaled to start. So until this semaphore is signaled, the GPU will not begin executing the command buffer.
            *
            * @param Semaphore A pointer to the semaphore to wait on.
            */
            void Wait(Semaphore* Semaphore);

            /*! \brief Checks if this command buffer is recording
            *
            * This returns true if Begin() has been called and End() has not been called. In any other case, it returns false.
            * 
            * @returns true if recording, false if not.
            */
            bool isRecording();

            /*! \brief Returns the command type, compute, graphics, or other. */
            Enums::CommandType GetCommandType() { return Type; };

            operator VkCommandBuffer() {
                return vkBuffer;
            }

            operator VkCommandBuffer*() {
                return &vkBuffer;
            }

            private:
            VkCommandBuffer vkBuffer; //! > The actual command buffer object
            Fence* pSignalFence; //! > A pointer to the fence to signal on completion.
            std::vector<Semaphore*> WaitSem; //! > The list of semaphores to wait on
            std::vector<Semaphore*> SignalSem; //! > The list of semaphores to signal

            VkQueue* cmdQueue; //! > The Queue to submit on
            VkCommandPool* pPool; //! > The command pool that allocated us.

            Enums::CommandType Type; //! > The type of command this buffer is (i.e. graphics, compute, etc.)

            bool bRecording; //! > Indicates whether or not this command buffer is recording or not.
        };

        /*! \brief A wrapper class for vulkan command pools. A device, queue, and queue family index is required to create one.
        */
        class CommandPool : VulkanObject
        {
            public:
            CommandPool(VkDevice& Device, VkQueue* pQueue, uint32_t QueueFamily);
            ~CommandPool();

            /*! \brief Creates and returns a command buffer. */
            CommandBuffer* CreateBuffer(Enums::CommandLevel cmdLevel);

            Enums::CommandType Type; //! > The type of command pool this is (e.g. if using a compute queue, this is compute).

            private:
            VkCommandPool vkPool;
            VkQueue* cmdQueue;
        };
    }
}