#pragma once

#include "Base/VulkanObject.hpp"
#include "Base/Allocator.hpp"

namespace VkMall
{
    class Buffer : VulkanObject
    {
      public:
        Buffer(VkDevice* pDevice, std::string Name = "Buffer") : VulkanObject(pDevice, Name) {}
        ~Buffer() { delete Alloc; vkDestroyBuffer(*pDevice, vkBuffer, nullptr); }

        Memory::Allocation* Alloc;
        VkBuffer vkBuffer;
    };
}