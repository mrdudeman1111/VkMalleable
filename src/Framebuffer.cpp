#include "Framebuffer.hpp"
#include "Base.hpp"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Wrappers
{
  Framebuffer::Framebuffer(VkDevice& Device, VkImage SwapchainImage, VkImageView SwapchainView, VkExtent2D FBSize, VkRenderPass Renderpass, FrameBufferDescription inDescription, uint32_t MemoryIndex) : VulkanObject(&Device, "Framebuffer")
  {
    VkResult Err;

    Description = inDescription;

    Images.push_back(SwapchainImage);
    ImageViews.push_back(SwapchainView);

    // we iterate over the FB description's Buffer descriptions -1 to account for the fact that one of the buffer descriptions is for the swapchain image and we don't need to make an image or view for that.
    for(uint32_t i = 0; i < Description.BufferCount-1; i++)
    {
      //Buffer Description index
      uint32_t DescIdx = i+1;

      VkImageUsageFlags Usage = 0;

      if(Description.BufferDescs[DescIdx].BufferTypes & Enums::eColor) Usage = Usage|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      if(Description.BufferDescs[DescIdx].BufferTypes & Enums::eDepth) Usage = Usage|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      if(Description.BufferDescs[DescIdx].BufferTypes & Enums::eInput) Usage = Usage|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
      if(Description.BufferDescs[DescIdx].BufferTypes & Enums::eSampled) Usage = Usage|VK_IMAGE_USAGE_SAMPLED_BIT;

      VkImageCreateInfo ImageCI{};
      ImageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      ImageCI.usage = Usage;
      ImageCI.extent = VkExtent3D{FBSize.width, FBSize.height, 1};
      ImageCI.format = Description.BufferDescs[DescIdx].Format;
      ImageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
      ImageCI.samples = Description.BufferDescs[DescIdx].Samples;
      ImageCI.imageType = VK_IMAGE_TYPE_2D;
      ImageCI.mipLevels = 1;
      ImageCI.arrayLayers = 1;
      ImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      ImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

      VkImage Img;

      if((Err = vkCreateImage(Device, &ImageCI, nullptr, &Img)) != VK_SUCCESS)
      {
        throw std::runtime_error("Wrappers : failed to create image\n");
      }

      VkMemoryRequirements MemReq;

      vkGetImageMemoryRequirements(Device, Img, &MemReq);

      VkMemoryAllocateInfo AllocInfo{};
      AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      AllocInfo.allocationSize = MemReq.size;
      AllocInfo.memoryTypeIndex = MemoryIndex;

      VkDeviceMemory Mem;

      vkAllocateMemory(Device, &AllocInfo, nullptr, &Mem);

      vkBindImageMemory(Device, Img, Mem, 0);

      VkImageViewCreateInfo ViewCI{};
      ViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      ViewCI.format = Description.BufferDescs[DescIdx].Format;
      ViewCI.image = Img;
      ViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
      ViewCI.components.r = VK_COMPONENT_SWIZZLE_R;
      ViewCI.components.g = VK_COMPONENT_SWIZZLE_G;
      ViewCI.components.b = VK_COMPONENT_SWIZZLE_B;
      ViewCI.components.a = VK_COMPONENT_SWIZZLE_A;
      ViewCI.subresourceRange.aspectMask = (Description.BufferDescs[DescIdx].BufferTypes & Enums::eDepth) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
      ViewCI.subresourceRange.baseMipLevel = 0;
      ViewCI.subresourceRange.baseArrayLayer = 0;
      ViewCI.subresourceRange.levelCount = 1;
      ViewCI.subresourceRange.layerCount = 1;

      VkImageView ImgView;

      if((Err = vkCreateImageView(Device, &ViewCI, nullptr, &ImgView)) != VK_SUCCESS)
      {
        throw std::runtime_error("Wrappers : Failed to create image views for Framebuffer\n");
      }

      Images.push_back(Img);
      ImageAllocations.push_back(Mem);
      ImageViews.push_back(ImgView);
    }

    VkFramebufferCreateInfo FrameBufferCI{};
    FrameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FrameBufferCI.renderPass = Renderpass;
    FrameBufferCI.attachmentCount = ImageViews.size();
    FrameBufferCI.pAttachments = ImageViews.data();
    FrameBufferCI.width = FBSize.width;
    FrameBufferCI.height = FBSize.height;
    FrameBufferCI.layers = 1;

    vkCreateFramebuffer(Device, &FrameBufferCI, nullptr, &vkFB);
  }

  Framebuffer::~Framebuffer()
  {
    vkDestroyFramebuffer(*pDevice, vkFB, nullptr);

    for(uint32_t i = 0; i < ImageViews.size(); i++)
    {
      vkDestroyImageView(*pDevice, ImageViews[i], nullptr);
    }

    // start at index 1 so we skip over the swapchain image. The swapchain image's lifetime is managed by the swapchain, not the framebuffer.
    for(uint32_t i = 1; i < Images.size(); i++)
    {
      vkDestroyImage(*pDevice, Images[i], nullptr);
    }

    for(uint32_t i = 0; i < ImageAllocations.size(); i++)
    {
      vkFreeMemory(*pDevice, ImageAllocations[i], nullptr);
    }
  }
}
