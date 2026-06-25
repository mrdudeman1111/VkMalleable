#pragma once

#include "vulkan/vulkan.h"

#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

/* 32 mb */
#define PAGE_SIZE 32000000

/* Forward declarations for everything in this file */
namespace Wrappers
{
  class VulkanObject;

  namespace Memory
  {
    class Allocation;
    struct Page;
    class Allocator;
  }

  namespace vulkan
  {
    class Buffer;
    class Texture;
  }

  class Fence;
  class Semaphore;
  class CommandBuffer;

  namespace Input
  {
    class Keyboard;
    class Mouse;
  }
}
/* Forward declarations for everything in this file */

namespace Wrappers
{
  namespace Enums
  {
    enum FrameBufferTypeBits
    {
      eColor = 0x01,
      eDepth = 0x02,
      eInput = 0x04,
      eSampled = 0x08
    };

    enum eMemoryType
    {
      eHost,
      eLocal
    };

    enum class eResourceType
    {
      eScene,
      eObject
    };

    enum class eShaderStage
    {
      eVertex = VK_SHADER_STAGE_VERTEX_BIT,
      eFragment = VK_SHADER_STAGE_FRAGMENT_BIT,
      eGeometry = VK_SHADER_STAGE_GEOMETRY_BIT
    };

    enum eInputRate
    {
      ePerVertex,
      ePerInstance
    };

    enum eAttributeType
    {
      fVec1 = VK_FORMAT_R32_SFLOAT,
      iVec1 = VK_FORMAT_R32_SINT,
      uVec1 = VK_FORMAT_R32_UINT,
      fVec2 = VK_FORMAT_R32G32_SFLOAT,
      iVec2 = VK_FORMAT_R32G32_SINT,
      uVec2 = VK_FORMAT_R32G32_UINT,
      fVec3 = VK_FORMAT_R32G32B32_SFLOAT,
      iVec3 = VK_FORMAT_R32G32B32_SINT,
      uVec3 = VK_FORMAT_R32G32B32_UINT,
      fVec4 = VK_FORMAT_R32G32B32A32_SFLOAT,
      iVec4 = VK_FORMAT_R32G32B32A32_SINT,
      uVec4 = VK_FORMAT_R32G32B32A32_UINT
    };

    enum class AttachmentType
    {
      eColorAtt = 1,
      eDepthAtt = 2,
      eInputAtt = 3,
      ePreserveAtt = 4,
      eResolveAtt = 5
    };


    enum eBufferUsage
    {

    };

    enum CommandType
    {
      eGraphics,
      eCompute,
      eTransfer
    };

    enum CommandLevel
    {
      ePrimary,
      eSecondary
    };
  }


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

  namespace Memory
  {
    //! A class that encapsulates an Allocation on a VkMemory Object.
    /*!
      This class wraps Allocations on shared VkMemory Obects. These VkDeviceMemory Objects are handled by Pages, which are managed by Allocators. The Interface for Allocators is implemented by the Allocator class. This struct handles binding of vulkan resources to it, it also stores information on Offset, size, Alignment requirements (retrieved from VkMemoryRequirements). Another struct is planned that will wrap this struct into an object usable by the allocator, then this struct will be passed to objects and runtimes requesting Allocations for resources, this struct will be stripped down quite a bit for convenience.
    */
    class Allocation : VulkanObject
    {
      friend Allocator;
      friend Page;

      public:
        Allocation(VkDevice* Device, Allocator* pAllocator, std::string AllocationName = "") : pAllocator(pAllocator), VulkanObject(Device, AllocationName) {}
        ~Allocation() { Free(); }

        void* Map();

        VkDeviceMemory Memory;
        VkDeviceSize Offset;
        VkDeviceSize Size;

        VkDeviceSize Alignment;
        Allocation* pNext;

      private:
        void Free();

        Allocator* pAllocator;

        void* pMemory = nullptr;

        // this enum is used to determine the method we use to transfer memory to this allocation (whether or not we can directly fill the buffer or use a staging buffer to transfer from RAM into VRAM)
        Enums::eMemoryType MemType;
    };

    //! struct wraps Pages on Device local memory, Host visible memory, etc .
    /*!
      This struct wraps VkDeviceMemory Objects in a page-like manner. We keep track of total size and currently available space, as well as a linked list consisting of all current allocations handled by this object. The Free method removes Allocations from the tracked alloc linked list. (Allocation and binding of resources is handled by Allocator, not Page).
    */
    struct Page : VulkanObject
    {
      public:
        Page(VkDevice* Device, std::string PageName = "Device Memory Page") : VulkanObject(Device, PageName) { bMapped = false; };
        ~Page() { vkFreeMemory(*pDevice, Memory, nullptr); }

        void* Map(size_t Offset);

        void Free(Allocation* pAllocation);

        VkDeviceMemory Memory;
        VkDeviceSize PageSize;
        VkDeviceSize FreeSpace;

        Allocation* HeadAllocation;

      private:
        void* pMemory;
        bool bMapped;
    };

    //! Interface class for allocators
    /*!
      Class that inherit and implement this Class must manage Available "Pages" (fancy VkDeviceMemory objects) and handle the allocation and deallocation of VkImage and VkBuffer objects. Allocators are not responsible for managing these resources' lifetime, but rather the lifetime of the Allocation/Memory objects that supports them. Copy operations are also handled, these can be handled however you choose to, but be cautious about data hazards (RAR, WAR, RAW, WAW hazards) in graphics/transfer/compute commands that are using the alocation.
    */
    class Allocator
    {
      public:
        Allocator() { DeviceLocalPages = {}; HostVisiblePages = {}; }
        ~Allocator() {}

        virtual vulkan::Buffer* CreateBuffer(size_t Size, VkBufferUsageFlags Usage = 0) = 0;
        virtual vulkan::Texture* CreateImage(VkExtent2D Size, VkFormat Format, VkImageUsageFlags Usage, VkSampleCountFlagBits Samples) = 0;

        virtual Allocation* Allocate(VkBuffer& Buffer, Enums::eMemoryType MemType) = 0;
        virtual Allocation* Allocate(VkImage& Image, Enums::eMemoryType MemType) = 0;

        virtual void Copy(VkBuffer* pDst, void* pData, size_t Size, VkDeviceSize dstOffset = 0) = 0;
        virtual void Copy(VkPipelineStageFlags DstStage, VkImage* pDst, void* pData, size_t Size, VkImageSubresourceLayers SubResource, VkExtent3D RegionExtent, VkOffset3D RegionOffset = {0, 0, 0}) = 0;

        void Free(Allocation* pAllocation);

        void* Map(Allocation* pAllocation);

      protected:
        std::vector<Page*> DeviceLocalPages;
        std::vector<Page*> HostVisiblePages;
    };
  }

  namespace vulkan
  {
    class Buffer : VulkanObject
    {
      public:
        Buffer(VkDevice* pDevice, std::string Name = "Buffer") : VulkanObject(pDevice, Name) {}
        ~Buffer() { delete Alloc; vkDestroyBuffer(*pDevice, vkBuffer, nullptr); }

        Memory::Allocation* Alloc;
        VkBuffer vkBuffer;
    };

    class Texture : VulkanObject
    {
      public:
        Texture(VkDevice* pDevice, std::string Name = "Texture") : VulkanObject(pDevice, Name) { Img = VK_NULL_HANDLE; View = VK_NULL_HANDLE; CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED; }

        ~Texture()
        {
          delete Alloc;
          if(Img != VK_NULL_HANDLE) vkDestroyImage(*pDevice, Img, nullptr);
          if(View != VK_NULL_HANDLE) vkDestroyImageView(*pDevice, View, nullptr);
        }

        VkImageMemoryBarrier CreateBarrier(VkImageLayout Layout);

        Memory::Allocation* Alloc;

        VkImage Img;
        VkImageLayout CurrentLayout;
        VkImageView View;
        VkExtent2D Extent;
    };
  }

  class Fence : VulkanObject
  {
    public:
      Fence(VkDevice* Device, bool bSignaled = false, std::string FenceName = "Fence");
      ~Fence();

      operator VkFence()
      {
        return vkFence;
      }

      operator VkFence*()
      {
        return &vkFence;
      }

      void Wait();

    private:
      VkFence vkFence;
  };

  class Semaphore : VulkanObject
  {
    public:
      Semaphore(VkDevice* Device, std::string SemaphoreName = "Semaphore");
      ~Semaphore();

      operator VkSemaphore()
      {
        return vkSemaphore;
      }

      operator VkSemaphore*()
      {
        return &vkSemaphore;
      }

    private:
      VkSemaphore vkSemaphore;
  };

  class CommandBuffer : VulkanObject
  {
    public:
      CommandBuffer(VkDevice& Device, VkCommandPool* AllocaterPool, VkQueue* pQueue, VkCommandBuffer pCmdBuffer, Enums::CommandType Type);
      ~CommandBuffer();

      void Reset();

      void Begin();
      void End();

      void Submit();

      void Signal(Fence* Fence);
      void Signal(Semaphore* Semaphore);
      void Wait(Semaphore* Semaphore);

      bool isRecording();

      Enums::CommandType GetCommandType() { return Type; };

      operator VkCommandBuffer() {
        return vkBuffer;
      }

      operator VkCommandBuffer*() {
        return &vkBuffer;
      }

    private:
      VkCommandBuffer vkBuffer;
      Fence* pSignalFence;
      std::vector<Semaphore*> WaitSem;
      std::vector<Semaphore*> SignalSem;

      VkQueue* cmdQueue;
      VkCommandPool* pPool;

      Enums::CommandType Type;

      bool bRecording;
  };
}

