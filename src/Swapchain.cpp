#include <cstdint>
#include <stdexcept>

#include "Swapchain.hpp"
#include "Base.hpp"

namespace Wrappers
{
  Swapchain::Swapchain(VkPhysicalDevice& PhysDevice, VkDevice* Device, uint32_t DeviceMemoryIdx, VkSurfaceKHR Surface, CommandBuffer* pCmdBuffer, VkExtent2D WindowSize) : LocalMemory(DeviceMemoryIdx), pBarrierBuffer(pCmdBuffer), SwapchainSize(WindowSize), VulkanObject(Device, "Swapchain")
  {
    VkResult Err;

    Framebuffers.resize(0);

    vkSurface = Surface;

    uint32_t FormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysDevice, Surface, &FormatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> SurfaceFormats(FormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysDevice, Surface, &FormatCount, SurfaceFormats.data());

    for(uint32_t i = 0; i < FormatCount; i++)
    {
      if(SurfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        Format = SurfaceFormats[i];
      }
    }

    VkSwapchainCreateInfoKHR SwapCI{};
    SwapCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapCI.clipped = VK_TRUE;
    SwapCI.surface = Surface;
    SwapCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    SwapCI.imageExtent = WindowSize;
    SwapCI.imageFormat = Format.format;
    SwapCI.imageColorSpace = Format.colorSpace;
    SwapCI.imageArrayLayers = 1;
    SwapCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    SwapCI.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    SwapCI.minImageCount = 3;
    SwapCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    SwapCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      
#ifdef __APPLE__
      SwapCI.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
#endif

    if((Err = vkCreateSwapchainKHR(*pDevice, &SwapCI, nullptr, &vkSwapchain)) != VK_SUCCESS)
    {
      throw std::runtime_error("Wrappers : Failed to create swapchain");
    }

    uint32_t SwapImageCount;
    vkGetSwapchainImagesKHR(*pDevice, vkSwapchain, &SwapImageCount, nullptr);
    SwapchainImages.resize(SwapImageCount);
    vkGetSwapchainImagesKHR(*pDevice, vkSwapchain, &SwapImageCount, SwapchainImages.data());

    VkFenceCreateInfo FenceCI{};
    FenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    vkCreateFence(*pDevice, &FenceCI, nullptr, &ImageFence);
  }

  Swapchain::~Swapchain()
  {
    for(uint32_t i = 0; i < Framebuffers.size(); i++)
    {
      delete Framebuffers[i];
    }

    delete pBarrierBuffer;

    vkDestroyFence(*pDevice, ImageFence, nullptr);

    vkDestroySwapchainKHR(*pDevice, vkSwapchain, nullptr);

    /* We don't need to delete the swapchain images or surface. The images are deleted with the Swapchain, the Swapchain image View(s) are deleted by the framebuffers, and the Surface's lifetime is not managed by the swapchain */
  }

  void Swapchain::CreateFrameBuffers(VkRenderPass* Renderpass, FrameBufferDescription Description)
  {
    VkResult Err;

    std::vector<VkImageMemoryBarrier> ImageBarriers(0);

    for(uint32_t i = 0; i < SwapchainImages.size(); i++)
    {
      VkImageView SwapchainView;

      VkImageViewCreateInfo ViewCI{};
      ViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      ViewCI.format = Format.format;
      ViewCI.image = SwapchainImages[i];
      ViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
      ViewCI.components.r = VK_COMPONENT_SWIZZLE_R;
      ViewCI.components.g = VK_COMPONENT_SWIZZLE_G;
      ViewCI.components.b = VK_COMPONENT_SWIZZLE_B;
      ViewCI.components.a = VK_COMPONENT_SWIZZLE_A;
      ViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      ViewCI.subresourceRange.layerCount = 1;
      ViewCI.subresourceRange.levelCount = 1;
      ViewCI.subresourceRange.baseArrayLayer = 0;
      ViewCI.subresourceRange.baseMipLevel = 0;

      if((Err = vkCreateImageView(*pDevice, &ViewCI, nullptr, &SwapchainView)) != VK_SUCCESS)
      {
        throw std::runtime_error("Wrappers : failed to create swapchain view for framebuffers\n");
      }

      Framebuffer* tmp = new Framebuffer(*pDevice, SwapchainImages[i], SwapchainView, SwapchainSize, *Renderpass, Description, LocalMemory);

      Framebuffers.push_back(tmp);

      for(uint32_t x = 0; x < Description.BufferCount; x++)
      {
        VkImageMemoryBarrier tmpBarrier{};
        tmpBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        tmpBarrier.image = (*Framebuffers[i])[x]; /* dereference the Framebuffer at index i and retrieve buffer at index x */
        tmpBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        tmpBarrier.newLayout = Description.BufferDescs[x].FirstLayout;
        tmpBarrier.srcAccessMask = VK_ACCESS_NONE;
        tmpBarrier.dstAccessMask = VK_ACCESS_NONE;

        if(Description.BufferDescs[x].BufferTypes & Enums::FrameBufferTypeBits::eDepth) tmpBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        else tmpBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        tmpBarrier.subresourceRange.layerCount = 1;
        tmpBarrier.subresourceRange.levelCount = 1;
        tmpBarrier.subresourceRange.baseMipLevel = 0;
        tmpBarrier.subresourceRange.baseArrayLayer = 0;

        ImageBarriers.push_back(tmpBarrier);
      }
    }

    VkCommandBufferBeginInfo BeginInfo{};
    BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    pBarrierBuffer->Begin();
      vkCmdPipelineBarrier(*pBarrierBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, ImageBarriers.size(), ImageBarriers.data());
    pBarrierBuffer->End();

    pBarrierBuffer->Submit();
  }

  Framebuffer* Swapchain::GetNextFramebuffer(uint32_t& Idx)
  {
    vkAcquireNextImageKHR(*pDevice, vkSwapchain, UINT32_MAX, VK_NULL_HANDLE, ImageFence, &Idx);

    vkWaitForFences(*pDevice, 1, &ImageFence, VK_TRUE, UINT64_MAX);
    vkResetFences(*pDevice, 1, &ImageFence);

    return Framebuffers[Idx];
  }
}

