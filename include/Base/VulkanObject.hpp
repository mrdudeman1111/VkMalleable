#pragma once

#include "vulkan/vulkan.h"
#include <iostream>

namespace VkMall
{
  //! Base object for vulkan Object Wrappers. construction and destruction of vulkan objects should be handled through constructor and destructor
  /*!
    Any classes that handle Vulkan Handles (aka VkDevice VkInstance or even VkSwapchainKHR) should inherit from this class. These classes should create any vulkan objects needed during construction and free them during destruction. During destruction, a message saying "Wrappers : Deleting {Object Name}" will appear in cout.
  */
  class VulkanObject
  {
    public:
      VulkanObject(VkDevice* Device, std::string Name = "(Uknown Vulkan Object)") : pDevice(Device), ObjectName(Name) {}
      ~VulkanObject() { std::cout << "Wrappers : Deleting " << ObjectName << '\n'; }

    protected:
      VkDevice* pDevice;
      std::string ObjectName;
  };
}