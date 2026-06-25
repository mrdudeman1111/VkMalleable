#pragma once

#include <vector>

#include "Framebuffer.hpp"

#include <GLFW/glfw3.h>

namespace Wrappers
{
  class Swapchain : VulkanObject
  {
    public:
      Swapchain(VkPhysicalDevice& PhysDevice, VkDevice* pDevice, uint32_t DeviceMemoryIdx, VkSurfaceKHR Surface, CommandBuffer* pCmdBuffer, VkExtent2D WindowSize);
      ~Swapchain();

      inline VkSwapchainKHR GetSwapchain() { return vkSwapchain; }
      inline VkSurfaceFormatKHR GetFormat() { return Format; }
      inline VkExtent2D GetExtent() { return SwapchainSize; }

      void CreateFrameBuffers(VkRenderPass* Renderpass, FrameBufferDescription Description);

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

