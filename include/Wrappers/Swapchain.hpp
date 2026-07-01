#pragma once

#include "Wrappers/Framebuffer.hpp"
#include "Wrappers/Commands.hpp"

namespace VkMall
{
    namespace Wrappers
    {
        class Swapchain : VulkanObject
        {
            public:
            Swapchain(VkPhysicalDevice& PhysDevice, VkDevice* pDevice, uint32_t DeviceMemoryIdx, VkSurfaceKHR Surface, Wrappers::CommandBuffer* pCmdBuffer, VkExtent2D WindowSize);
            ~Swapchain();

            inline VkSwapchainKHR GetSwapchain() { return vkSwapchain; }
            inline VkSurfaceFormatKHR GetFormat() { return Format; }
            inline VkExtent2D GetExtent() { return SwapchainSize; }

            void CreateFrameBuffers(VkRenderPass* Renderpass, Wrappers::FrameBufferDescription Description);

            Framebuffer* GetNextFramebuffer(uint32_t& Idx);

            operator VkSwapchainKHR()
            {
                return vkSwapchain;
            }

            operator VkSwapchainKHR*()
            {
                return &vkSwapchain;
            }

            std::vector<VkImage> SwapchainImages;

            private:
            VkSwapchainKHR vkSwapchain;
            VkSurfaceKHR vkSurface;

            VkExtent2D SwapchainSize;
            VkSurfaceFormatKHR Format;

            CommandBuffer* pBarrierBuffer;

            uint32_t LocalMemory;

            VkFence ImageFence;

            std::vector<Framebuffer*> Framebuffers;
        };
    }
}
