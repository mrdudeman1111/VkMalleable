#pragma once

#include "Base/Memory.hpp"

namespace VkMall
{
    namespace Base
    {
        class Texture : VulkanObject
        {
        public:
            Texture(VkDevice* pDevice, std::string Name = "Texture") : VulkanObject(pDevice, Name) { Img = VK_NULL_HANDLE; View = VK_NULL_HANDLE; CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED; }

            ~Texture()
            {
                delete Alloc;

                if(Img != VK_NULL_HANDLE)
                    vkDestroyImage(*pDevice, Img, nullptr);
                if(View != VK_NULL_HANDLE)
                    vkDestroyImageView(*pDevice, View, nullptr);
            }

            /*! \brief Creates a memory barrier for transitioning to a layout.
            *
            * Creates a memory barrier used for transitions to certain layouts. To use this object, take a recording command buffer with a compute or graphics type and call vkCmdPipelineBarrier() and pass this object with any other memory barriers you may have. Then end and submit the command buffer. This will transition this texture from an UNDEFINED layout to whatever layout you passed in this function.
            * 
            * @param Layout The image layout to transition to.
            */
            VkImageMemoryBarrier CreateBarrier(VkImageLayout Layout);

            Base::Allocation* Alloc;

            VkImage Img;
            VkImageLayout CurrentLayout;
            VkImageView View;
            VkExtent2D Extent;
        };
    }
}