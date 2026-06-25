#pragma once

#include "Base.hpp"

#include <vector>

namespace Wrappers
{
  struct BufferDescription
  {
    public:
      VkImageLayout FirstLayout;
      VkImageLayout FinalLayout;

      VkAttachmentStoreOp StorageOp;
      VkAttachmentLoadOp LoadOp;

      Enums::FrameBufferTypeBits BufferTypes;
      VkSampleCountFlagBits Samples;
      VkFormat Format;
  };

  struct FrameBufferDescription
  {
    public:
      uint32_t BufferCount;
      std::vector<BufferDescription> BufferDescs;
  };

  class Framebuffer : VulkanObject
  {
    public:
      Framebuffer(VkDevice& Device, VkImage SwapchainImage, VkImageView SwapchainView, VkExtent2D FBSize, VkRenderPass Renderpass, FrameBufferDescription inDescription, uint32_t MemoryIndex);
      ~Framebuffer();

      operator VkFramebuffer() { return vkFB; }
      operator VkFramebuffer*() { return &vkFB; }

      VkImage operator [](size_t Index) {
        return Images[Index];
      }

      FrameBufferDescription GetDescription() { return Description; }

    private:
      std::vector<VkDeviceMemory> ImageAllocations;
      std::vector<VkImage> Images;
      std::vector<VkImageView> ImageViews;

      VkImageView SwapchainView;

      VkFramebuffer vkFB;

      FrameBufferDescription Description;
  };
}

