#pragma once

#include "Base/Enums.hpp"
#include "Base/VulkanObject.hpp"

/* 32 mb */
#define PAGE_SIZE 32000000

namespace VkMall
{
    namespace Base
    {
        // Forward declared for allocator.
        class Buffer;
        class Texture;

        class Allocator;

        //! A class that encapsulates an Allocation on a VkMemory Object.
        /*!
        This class wraps Allocations on shared VkMemory Obects. These VkDeviceMemory Objects are handled by Pages, which are managed by Allocators. The Interface for Allocators is implemented by the Allocator class. This struct handles binding of vulkan resources to it, it also stores information on Offset, size, Alignment requirements (retrieved from VkMemoryRequirements). Another struct is planned that will wrap this struct into an object usable by the allocator, then this struct will be passed to objects and runtimes requesting Allocations for resources, this struct will be stripped down quite a bit for convenience.
        */
        class Allocation : VulkanObject
        {
            friend Allocator;
            // friend Page;

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

            virtual Buffer* CreateBuffer(size_t Size, VkBufferUsageFlags Usage = 0) = 0;
            virtual Texture* CreateImage(VkExtent2D Size, VkFormat Format, VkImageUsageFlags Usage, VkSampleCountFlagBits Samples) = 0;

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
}
