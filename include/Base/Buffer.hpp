#pragma once

#include "Base/Memory.hpp"

namespace VkMall
{
    namespace Base
    {
        class Buffer : VulkanObject
        {
        public:
            Buffer(VkDevice* pDevice, std::string Name = "Buffer") : VulkanObject(pDevice, Name) {}
            ~Buffer() { delete Alloc; vkDestroyBuffer(*pDevice, vkBuffer, nullptr); }

            operator VkBuffer()
            {
                return vkBuffer;
            }

            operator VkBuffer*()
            {
                return &vkBuffer;
            }

            Base::Allocation* Alloc;
            VkBuffer vkBuffer;
        };
    }
}
