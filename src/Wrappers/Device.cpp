#include "Wrappers/Device.hpp"

#include "Wrappers/Swapchain.hpp"
#include "Wrappers/Framebuffer.hpp"
#include "Wrappers/Mesh.hpp"

namespace VkMall
{
    namespace Wrappers
    {
        Device::Device(VkPhysicalDevice PhysDev, VkSurfaceKHR RenderSurface, uint32_t ExtensionCount, const char** pExtensions, VkPhysicalDeviceFeatures Feat) : PhysicalDevice(PhysDev)
        {
            VkResult Err;
            
            uint32_t QueuefamilyCount;
            vkGetPhysicalDeviceQueueFamilyProperties(PhysDev, &QueuefamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> FamilyProperties(QueuefamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(PhysDev, &QueuefamilyCount, FamilyProperties.data());
            
            Surface = RenderSurface;
            
            Families.GraphicsFamily = 999;
            Families.ComputeFamily = 999;
            Families.TransferFamily = 999;
            
            for(uint32_t i = 0; i < QueuefamilyCount; i++)
            {
                // we use the FamilyProperties::queueCount as an iterator to indicate how many queues remain available
                if(FamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && Families.GraphicsFamily == 999 && FamilyProperties[i].queueCount > 0)
                {
                    Families.GraphicsFamily = i;
                    FamilyProperties[i].queueCount--;
                }
                if(FamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT && Families.ComputeFamily == 999 && FamilyProperties[i].queueCount > 0)
                {
                    Families.ComputeFamily = i;
                    FamilyProperties[i].queueCount--;
                }
                if(FamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT && Families.TransferFamily == 999 && FamilyProperties[i].queueCount > 0)
                {
                    Families.TransferFamily = i;
                    FamilyProperties[i].queueCount--;
                }
            }
            
            VkPhysicalDeviceMemoryProperties MemProps;
            vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemProps);
            
            Memory.VisibleMemory = 999;
            Memory.LocalMemory = 999;
            
            for(uint32_t i = 0; i < MemProps.memoryTypeCount; i++)
            {
                if(MemProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT && Memory.LocalMemory == 999)
                {
                    Memory.LocalMemory = i;
                }
                else if(MemProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT && Memory.VisibleMemory == 999)
                {
                    Memory.VisibleMemory = i;
                }
            }
            
            VkDeviceQueueCreateInfo QueuesCI[3] = {};
            float Prio[] = {1.f, 1.f};

            QueuesCI[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            QueuesCI[0].queueCount = 1;
            QueuesCI[0].queueFamilyIndex = Families.GraphicsFamily;
            QueuesCI[0].pQueuePriorities = Prio;
            
            QueuesCI[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            QueuesCI[1].queueCount = 1;
            QueuesCI[1].queueFamilyIndex = Families.ComputeFamily;
            QueuesCI[1].pQueuePriorities = Prio;
            
            if(Families.TransferFamily != 999)
            {
                QueuesCI[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                QueuesCI[2].queueCount = 1;
                QueuesCI[2].queueFamilyIndex = Families.TransferFamily;
                QueuesCI[2].pQueuePriorities = Prio;
            }
            
            Extensions.push_back("VK_KHR_swapchain");
            
            for(uint32_t i = 0; i < ExtensionCount; i++)
            {
                if(!strcmp(pExtensions[i], "VK_KHR_swapchain"))
                {
                    continue;
                }
                Extensions.push_back(pExtensions[i]);
            }
            
        #ifdef __APPLE__
            Extensions.push_back("VK_KHR_portability_subset");
        #endif
            
            VkDeviceCreateInfo DeviceCI{};
            DeviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            DeviceCI.enabledExtensionCount = (uint32_t)Extensions.size();
            DeviceCI.ppEnabledExtensionNames = Extensions.data();
            DeviceCI.queueCreateInfoCount = 3;
            DeviceCI.pQueueCreateInfos = QueuesCI;
            DeviceCI.pEnabledFeatures = &Feat;
            
            Features = Feat;
            
            if((Err = vkCreateDevice(PhysDev, &DeviceCI, nullptr, &vkDevice)) != VK_SUCCESS)
            {
                throw std::runtime_error("Wrappers : Failed to create device\n");
            }
            
            vkGetDeviceQueue(vkDevice, Families.GraphicsFamily, 0, &Queues.GraphicsQueue);
            vkGetDeviceQueue(vkDevice, Families.ComputeFamily, 0, &Queues.ComputeQueue);
            
            if(Families.TransferFamily != 999)
            {
                vkGetDeviceQueue(vkDevice, Families.TransferFamily, 0, &Queues.TransferQueue);
            }
            
            cmdGraphicsPool = CreateCommandPool(Enums::eGraphics);
            
            TransferAlloc = nullptr;
            TransferBuffer = nullptr;
            cmdTransferPool = nullptr;
            cmdTransferBuffer = nullptr;
            pTransferFence = new Fence(&vkDevice);
            pRenderFence = new Fence(&vkDevice, true);
            pDescriptorAllocator = new DescriptorAllocator(&vkDevice, this);
        }

        Device::~Device()
        {
            if(pSceneDescriptors[0] != nullptr)
            {
                delete pSceneDescriptors[0];
                delete pSceneDescriptors[1];
                delete pSceneDescriptors[2];
            }
            
            delete pDescriptorAllocator;
            delete pRenderFence;
            delete pTransferFence;
            if(cmdTransferBuffer != nullptr) delete cmdTransferBuffer;
            if(cmdTransferPool != nullptr) delete cmdTransferPool;
            if(TransferBuffer != nullptr) vkDestroyBuffer(vkDevice, TransferBuffer, nullptr);
            if(TransferAlloc != nullptr) delete TransferAlloc;
            vkDestroyDevice(vkDevice, nullptr);
        }

        Base::Buffer* Device::CreateBuffer(size_t Size, VkBufferUsageFlags Usage)
        {
            VkResult Err;
            
            VkBufferCreateInfo BufferCI{};
            BufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            BufferCI.size = Size;
            BufferCI.usage = Usage;
            BufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            
            Base::Buffer* Ret = new Base::Buffer(&vkDevice);
            
            if((Err = vkCreateBuffer(vkDevice, &BufferCI, nullptr, &Ret->vkBuffer)) != VK_SUCCESS)
            {
                throw std::runtime_error("Wrappers : Failed to create buffer");
            }
            
            return Ret;
        }

        Base::Texture* Device::CreateImage(VkExtent2D Extent, VkFormat Format, VkImageUsageFlags Usage, VkSampleCountFlagBits Samples)
        {
            VkResult Err;
            
            Base::Texture* pRet = new Base::Texture(&vkDevice);
            
            VkImageCreateInfo ImageCI{};
            ImageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            ImageCI.usage = Usage;
            ImageCI.imageType = VK_IMAGE_TYPE_2D;
            ImageCI.format = Format;
            ImageCI.extent = {Extent.width, Extent.height, 1};
            ImageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
            ImageCI.samples = Samples;
            ImageCI.mipLevels = 0;
            ImageCI.arrayLayers = 1;
            ImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            
            if((Err = vkCreateImage(vkDevice, &ImageCI, nullptr, &pRet->Img)) != VK_SUCCESS)
            {
                throw std::runtime_error("Wrappers : Failed to create image\n");
            }
            
            return pRet;
        }

        Base::Allocation* Device::Allocate(VkBuffer& Buffer, Enums::eMemoryType MemType)
        {
            VkMemoryRequirements MemReq{};
            vkGetBufferMemoryRequirements(vkDevice, Buffer, &MemReq);
            
            // if the allocation is too large for a standard page.
            if(MemReq.size > PAGE_SIZE)
            {
                Base::Page* pTmp = new Base::Page(&vkDevice);
                
                // Configuration for a specialized page large enough to handle the allocation size.
                VkMemoryAllocateInfo AllocInfo{};
                AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                AllocInfo.allocationSize = MemReq.size;
                AllocInfo.memoryTypeIndex = (MemType == Enums::eHost) ? Memory.VisibleMemory : Memory.LocalMemory;
                
                if(vkAllocateMemory(vkDevice, &AllocInfo, nullptr, &pTmp->Memory) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : failed to allocate memory page for host-visible allocation");
                }
                
                Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);
                newAlloc->Memory = pTmp->Memory;
                newAlloc->Size = MemReq.size;
                newAlloc->Alignment = MemReq.alignment;
                newAlloc->Offset = 0;
                newAlloc->pNext = nullptr;
                
                if(vkBindBufferMemory(vkDevice, Buffer, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : Allocate(buffer) : failed to bind buffer to allocation memory\n");
                }
                
                pTmp->HeadAllocation = newAlloc;
                pTmp->FreeSpace = 0;
                pTmp->PageSize = MemReq.size;
                
                (MemType == Enums::eHost) ? HostVisiblePages.push_back(pTmp) : DeviceLocalPages.push_back(pTmp);
                
                return newAlloc;
            }
            
            if(MemType == Enums::eHost)
            {
                for(uint32_t i = 0; i < HostVisiblePages.size(); i++)
                {
                    if(HostVisiblePages[i]->FreeSpace > MemReq.size)
                    {
                        Base::Allocation* Iterator = HostVisiblePages[i]->HeadAllocation;
                        
                        while(Iterator)
                        {
                            if(Iterator->pNext)
                            {
                                if(Iterator->pNext->Offset - (Iterator->Offset+Iterator->Size) > MemReq.size+32)
                                {
                                    // find allocation position in memory and align with alignment requirements
                                    uint32_t AllocPosition = Iterator->Offset+Iterator->Size;
                                    AllocPosition += MemReq.alignment-(AllocPosition%MemReq.alignment);
                                    
                                    Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);
                                    
                                    // Insert new allocation into allocation (linked) list
                                    {
                                        newAlloc->pNext = Iterator->pNext;
                                        
                                        newAlloc->Memory = HostVisiblePages[i]->Memory;
                                        newAlloc->Size = MemReq.size;
                                        newAlloc->Alignment = MemReq.alignment;
                                        newAlloc->Offset = AllocPosition;
                                        
                                        Iterator->pNext = newAlloc;
                                    }
                                    
                                    HostVisiblePages[i]->FreeSpace = HostVisiblePages[i]->PageSize - (newAlloc->Offset+newAlloc->Size);
                                    
                                    if(vkBindBufferMemory(vkDevice, Buffer, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                                    {
                                        throw std::runtime_error("Wrappers : Allocate(Buffer) : failed to bind buffer to allocation memory\n");
                                    }
                                    
                                    return newAlloc;
                                }
                            }
                            else
                            {
                                if(HostVisiblePages[i]->PageSize-(Iterator->Offset+Iterator->Size) > MemReq.size)
                                {
                                    // find allocation position in memory and align with alignment requirements
                                    uint32_t AllocPosition = Iterator->Offset+Iterator->Size;
                                    AllocPosition += MemReq.alignment-(AllocPosition%MemReq.alignment);
                                    
                                    Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);

                                    // Insert new allocation into allocation (linked) list
                                    {
                                        newAlloc->pNext = Iterator->pNext;
                                        
                                        newAlloc->Memory = HostVisiblePages[i]->Memory;
                                        newAlloc->Size = MemReq.size;
                                        newAlloc->Alignment = MemReq.alignment;
                                        newAlloc->Offset = AllocPosition;
                                        
                                        Iterator->pNext = newAlloc;
                                    }
                                    
                                    HostVisiblePages[i]->FreeSpace = HostVisiblePages[i]->PageSize - (newAlloc->Offset+newAlloc->Size);
                                    
                                    if(vkBindBufferMemory(vkDevice, Buffer, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                                    {
                                        throw std::runtime_error("Wrappers : Allocate(Buffer) : failed to bind buffer to allocation memory\n");
                                    }
                                    
                                    return newAlloc;
                                }
                            }
                            
                            Iterator = Iterator->pNext;
                        }
                    }
                }
                
                Base::Page* newPage = new Base::Page(&vkDevice);
                
                VkMemoryAllocateInfo AllocInfo{};
                AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                AllocInfo.allocationSize = PAGE_SIZE;
                AllocInfo.memoryTypeIndex = Memory.VisibleMemory;
                
                if(vkAllocateMemory(vkDevice, &AllocInfo, nullptr, &newPage->Memory) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : failed to allocate memory for new host visible page\n");
                }
                
                Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);
                
                newAlloc->pNext = nullptr;
                newAlloc->Memory = newPage->Memory;
                newAlloc->Size = MemReq.size;
                newAlloc->Alignment = MemReq.alignment;
                newAlloc->Offset = 0;
                
                newPage->HeadAllocation = newAlloc;
                newPage->PageSize = PAGE_SIZE;
                newPage->FreeSpace = newPage->PageSize - (newAlloc->Offset+newAlloc->Size);
                
                if(vkBindBufferMemory(vkDevice, Buffer, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : Allocate(Buffer) : failed to bind buffer to allocation memory\n");
                }
                
                HostVisiblePages.push_back(newPage);
                
                return HostVisiblePages.back()->HeadAllocation;
            }
            else
            {
                for(uint32_t i = 0; i < DeviceLocalPages.size(); i++)
                {
                    if(DeviceLocalPages[i]->FreeSpace > MemReq.size)
                    {
                        Base::Allocation* Iterator = DeviceLocalPages[i]->HeadAllocation;
                        
                        while(Iterator)
                        {
                            if(Iterator->pNext)
                            {
                                if(Iterator->pNext->Offset - (Iterator->Offset+Iterator->Size) > MemReq.size+32)
                                {
                                    // find allocation position in memory and align with alignment requirements
                                    uint32_t AllocPosition = Iterator->Offset+Iterator->Size;
                                    AllocPosition += MemReq.alignment-(AllocPosition%MemReq.alignment);
                                    
                                    Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);
                                    
                                    // Insert new allocation into allocation (linked) list
                                    {
                                        newAlloc->pNext = Iterator->pNext;
                                        
                                        newAlloc->Memory = DeviceLocalPages[i]->Memory;
                                        newAlloc->Size = MemReq.size;
                                        newAlloc->Alignment = MemReq.alignment;
                                        newAlloc->Offset = AllocPosition;
                                        
                                        Iterator->pNext = newAlloc;
                                    }
                                    
                                    DeviceLocalPages[i]->FreeSpace = DeviceLocalPages[i]->PageSize - (newAlloc->Offset+newAlloc->Size);
                                    
                                    if(vkBindBufferMemory(vkDevice, Buffer, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                                    {
                                        throw std::runtime_error("Wrappers : Allocate(Buffer) : failed to bind buffer to allocation memory\n");
                                    }
                                    
                                    return newAlloc;
                                }
                            }
                            else
                            {
                                if(DeviceLocalPages[i]->PageSize-(Iterator->Offset+Iterator->Size) > MemReq.size)
                                {
                                    // find allocation position in memory and align with alignment requirements
                                    uint32_t AllocPosition = Iterator->Offset+Iterator->Size;
                                    AllocPosition += MemReq.alignment-(AllocPosition%MemReq.alignment);
                                    
                                    Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);
                                    
                                    // Insert new allocation into allocation (linked) list
                                    {
                                        newAlloc->pNext = Iterator->pNext;
                                        
                                        newAlloc->Memory = DeviceLocalPages[i]->Memory;
                                        newAlloc->Size = MemReq.size;
                                        newAlloc->Alignment = MemReq.alignment;
                                        newAlloc->Offset = AllocPosition;
                                        
                                        Iterator->pNext = newAlloc;
                                    }
                                    
                                    DeviceLocalPages[i]->FreeSpace = DeviceLocalPages[i]->PageSize - (newAlloc->Offset+newAlloc->Size);
                                    
                                    if(vkBindBufferMemory(vkDevice, Buffer, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                                    {
                                        throw std::runtime_error("Wrappers : Allocate(Buffer) : failed to bind buffer to allocation memory\n");
                                    }
                                    
                                    return newAlloc;
                                    
                                }
                            }
                            
                            Iterator = Iterator->pNext;
                        }
                    }
                }
                
                Base::Page* newPage = new Base::Page(&vkDevice);
                
                VkMemoryAllocateInfo AllocInfo{};
                AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                AllocInfo.allocationSize = PAGE_SIZE;
                AllocInfo.memoryTypeIndex = Memory.LocalMemory;
                
                if(vkAllocateMemory(vkDevice, &AllocInfo, nullptr, &newPage->Memory) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : failed to allocate memory for new host visible page\n");
                }
                
                Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);
                
                newAlloc->pNext = nullptr;
                newAlloc->Memory = newPage->Memory;
                newAlloc->Size = MemReq.size;
                newAlloc->Alignment = MemReq.alignment;
                newAlloc->Offset = 0;
                
                newPage->HeadAllocation = newAlloc;
                newPage->PageSize = PAGE_SIZE;
                newPage->FreeSpace = newPage->PageSize - (newAlloc->Offset+newAlloc->Size);
                
                if(vkBindBufferMemory(vkDevice, Buffer, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : Allocate(Buffer) : failed to bind buffer to allocation memory\n");
                }
                
                DeviceLocalPages.push_back(newPage);
                
                return DeviceLocalPages.back()->HeadAllocation;
            }
        }

        void Device::Allocate(Base::Buffer* pBuffer, Enums::eMemoryType MemType)
        {
            pBuffer->Alloc = Allocate(pBuffer->vkBuffer, MemType);
            return;
        }

        Base::Allocation* Device::Allocate(VkImage& Image, Enums::eMemoryType MemType)
        {
            VkMemoryRequirements MemReq{};
            vkGetImageMemoryRequirements(vkDevice, Image, &MemReq);
            
            if(MemReq.size > PAGE_SIZE)
            {
                Base::Page* pTmp = new Base::Page(&vkDevice);
                
                VkMemoryAllocateInfo AllocInfo{};
                AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                AllocInfo.allocationSize = MemReq.size;
                AllocInfo.memoryTypeIndex = (MemType == Enums::eHost) ? Memory.VisibleMemory : Memory.LocalMemory;
                
                if(vkAllocateMemory(vkDevice, &AllocInfo, nullptr, &pTmp->Memory) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : failed to allocate memory page for host-visible allocation");
                }
                
                Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);
                newAlloc->Memory = pTmp->Memory;
                newAlloc->Size = MemReq.size;
                newAlloc->Alignment = MemReq.alignment;
                newAlloc->Offset = 0;
                newAlloc->pNext = nullptr;
                
                if(vkBindImageMemory(vkDevice, Image, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : Allocate(buffer) : failed to bind buffer to allocation memory\n");
                }
                
                pTmp->HeadAllocation = newAlloc;
                pTmp->FreeSpace = 0;
                pTmp->PageSize = MemReq.size;
                
                (MemType == Enums::eHost) ? HostVisiblePages.push_back(pTmp) : DeviceLocalPages.push_back(pTmp);
                
                return newAlloc;
            }
            
            if(MemType == Enums::eHost)
            {
                for(uint32_t i = 0; i < HostVisiblePages.size(); i++)
                {
                    if(HostVisiblePages[i]->FreeSpace > MemReq.size)
                    {
                        Base::Allocation* Iterator = HostVisiblePages[i]->HeadAllocation;
                        
                        while(Iterator)
                        {
                            if(Iterator->pNext)
                            {
                                if(Iterator->pNext->Offset - (Iterator->Offset+Iterator->Size) > MemReq.size+32)
                                {
                                    // find allocation position in memory and align with alignment requirements
                                    uint32_t AllocPosition = Iterator->Offset+Iterator->Size;
                                    AllocPosition = AllocPosition+(AllocPosition%MemReq.alignment);
                                    
                                    Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);
                                    
                                    // Insert new allocation into allocation (linked) list
                                    {
                                        newAlloc->pNext = Iterator->pNext;
                                        
                                        newAlloc->Memory = HostVisiblePages[i]->Memory;
                                        newAlloc->Size = MemReq.size;
                                        newAlloc->Alignment = MemReq.alignment;
                                        newAlloc->Offset = AllocPosition;
                                        
                                        Iterator->pNext = newAlloc;
                                    }
                                    
                                    HostVisiblePages[i]->FreeSpace = HostVisiblePages[i]->PageSize - (newAlloc->Offset+newAlloc->Size);
                                    
                                    if(vkBindImageMemory(vkDevice, Image, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                                    {
                                        throw std::runtime_error("Wrappers : Allocate(Buffer) : failed to bind buffer to allocation memory\n");
                                    }
                                    
                                    return newAlloc;
                                }
                            }
                            else
                            {
                                if(HostVisiblePages[i]->PageSize-(Iterator->Offset+Iterator->Size) > MemReq.size)
                                {
                                    // find allocation position in memory and align with alignment requirements
                                    uint32_t AllocPosition = Iterator->Offset+Iterator->Size;
                                    AllocPosition = AllocPosition+(AllocPosition%MemReq.alignment);
                                    
                                    Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);
                                    
                                    // Insert new allocation into allocation (linked) list
                                    {
                                        newAlloc->pNext = Iterator->pNext;
                                        
                                        newAlloc->Memory = HostVisiblePages[i]->Memory;
                                        newAlloc->Size = MemReq.size;
                                        newAlloc->Alignment = MemReq.alignment;
                                        newAlloc->Offset = AllocPosition;
                                        
                                        Iterator->pNext = newAlloc;
                                    }
                                    
                                    HostVisiblePages[i]->FreeSpace = HostVisiblePages[i]->PageSize - (newAlloc->Offset+newAlloc->Size);
                                    
                                    if(vkBindImageMemory(vkDevice, Image, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                                    {
                                        throw std::runtime_error("Wrappers : Allocate(Buffer) : failed to bind buffer to allocation memory\n");
                                    }
                                    
                                    return newAlloc;
                                }
                            }
                            
                            Iterator = Iterator->pNext;
                        }
                    }
                }
                
                Base::Page* newPage = new Base::Page(&vkDevice);
                
                VkMemoryAllocateInfo AllocInfo{};
                AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                AllocInfo.allocationSize = PAGE_SIZE;
                AllocInfo.memoryTypeIndex = Memory.VisibleMemory;
                
                if(vkAllocateMemory(vkDevice, &AllocInfo, nullptr, &newPage->Memory) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : failed to allocate memory for new host visible page\n");
                }
                
                Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);
                
                newAlloc->pNext = nullptr;
                newAlloc->Memory = newPage->Memory;
                newAlloc->Size = MemReq.size;
                newAlloc->Alignment = MemReq.alignment;
                newAlloc->Offset = 0;
                
                newPage->HeadAllocation = newAlloc;
                newPage->PageSize = PAGE_SIZE;
                newPage->FreeSpace = newPage->PageSize - (newAlloc->Offset+newAlloc->Size);
                
                if(vkBindImageMemory(vkDevice, Image, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : Allocate(Buffer) : failed to bind buffer to allocation memory\n");
                }
                
                HostVisiblePages.push_back(newPage);
                
                return HostVisiblePages.back()->HeadAllocation;
            }
            else
            {
                for(uint32_t i = 0; i < DeviceLocalPages.size(); i++)
                {
                    if(DeviceLocalPages[i]->FreeSpace > MemReq.size)
                    {
                        Base::Allocation* Iterator = DeviceLocalPages[i]->HeadAllocation;
                        
                        while(Iterator)
                        {
                            if(Iterator->pNext)
                            {
                                if(Iterator->pNext->Offset - (Iterator->Offset+Iterator->Size) > MemReq.size+32)
                                {
                                    // find allocation position in memory and align with alignment requirements
                                    uint32_t AllocPosition = Iterator->Offset+Iterator->Size;
                                    AllocPosition = AllocPosition+(AllocPosition%MemReq.alignment);
                                    
                                    Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);
                                    
                                    // Insert new allocation into allocation (linked) list
                                    {
                                        newAlloc->pNext = Iterator->pNext;
                                        
                                        newAlloc->Memory = DeviceLocalPages[i]->Memory;
                                        newAlloc->Size = MemReq.size;
                                        newAlloc->Alignment = MemReq.alignment;
                                        newAlloc->Offset = AllocPosition;
                                        
                                        Iterator->pNext = newAlloc;
                                    }
                                    
                                    DeviceLocalPages[i]->FreeSpace = DeviceLocalPages[i]->PageSize - (newAlloc->Offset+newAlloc->Size);
                                    
                                    if(vkBindImageMemory(vkDevice, Image, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                                    {
                                        throw std::runtime_error("Wrappers : Allocate(Buffer) : failed to bind buffer to allocation memory\n");
                                    }
                                    
                                    return newAlloc;
                                }
                            }
                            else
                            {
                                if(DeviceLocalPages[i]->PageSize-(Iterator->Offset+Iterator->Size) > MemReq.size)
                                {
                                    // find allocation position in memory and align with alignment requirements
                                    uint32_t AllocPosition = Iterator->Offset+Iterator->Size;
                                    AllocPosition = AllocPosition+(AllocPosition%MemReq.alignment);
                                    
                                    Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);
                                    
                                    // Insert new allocation into allocation (linked) list
                                    {
                                        newAlloc->pNext = Iterator->pNext;
                                        
                                        newAlloc->Memory = DeviceLocalPages[i]->Memory;
                                        newAlloc->Size = MemReq.size;
                                        newAlloc->Alignment = MemReq.alignment;
                                        newAlloc->Offset = AllocPosition;
                                        
                                        Iterator->pNext = newAlloc;
                                    }
                                    
                                    DeviceLocalPages[i]->FreeSpace = DeviceLocalPages[i]->PageSize - (newAlloc->Offset+newAlloc->Size);
                                    
                                    if(vkBindImageMemory(vkDevice, Image, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                                    {
                                        throw std::runtime_error("Wrappers : Allocate(Buffer) : failed to bind buffer to allocation memory\n");
                                    }
                                    
                                    return newAlloc;
                                    
                                }
                            }
                            
                            Iterator = Iterator->pNext;
                        }
                    }
                }
                
                Base::Page* newPage = new Base::Page(&vkDevice);
                
                VkMemoryAllocateInfo AllocInfo{};
                AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                AllocInfo.allocationSize = PAGE_SIZE;
                AllocInfo.memoryTypeIndex = Memory.LocalMemory;
                
                if(vkAllocateMemory(vkDevice, &AllocInfo, nullptr, &newPage->Memory) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : failed to allocate memory for new host visible page\n");
                }
                
                Base::Allocation* newAlloc = new Base::Allocation(&vkDevice, (Base::Allocator*)this);
                
                newAlloc->pNext = nullptr;
                newAlloc->Memory = newPage->Memory;
                newAlloc->Size = MemReq.size;
                newAlloc->Alignment = MemReq.alignment;
                newAlloc->Offset = 0;
                
                newPage->HeadAllocation = newAlloc;
                newPage->PageSize = PAGE_SIZE;
                newPage->FreeSpace = newPage->PageSize - (newAlloc->Offset+newAlloc->Size);
                
                if(vkBindImageMemory(vkDevice, Image, newAlloc->Memory, newAlloc->Offset) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : Allocate(Buffer) : failed to bind buffer to allocation memory\n");
                }
                
                DeviceLocalPages.push_back(newPage);
                
                return newAlloc;
            }
        }

        void Device::Copy(VkBuffer* pDst, void* pData, size_t Size, VkDeviceSize dstOffset)
        {
			if(bTransfering)
			{
				// std::cout << "made a copy call during an active transfer.\n";
			}

            // if not allocated already, create a 192000000 volume for transfer operations
            if(TransferAlloc == nullptr)
            {
                VkBufferCreateInfo BufferCI{};
                BufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                BufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                BufferCI.size = 192000000;
                BufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                
                if(vkCreateBuffer(vkDevice, &BufferCI, nullptr, &TransferBuffer) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : failed to allocate transfer buffer for copy()\n");
                }

                TransferAlloc = Allocate(TransferBuffer, Enums::eHost);

                pTransferMemory = TransferAlloc->Map();
            }
            
            memcpy(((uint8_t*)pTransferMemory)+TransferTail, pData, Size);

            BufferCopyInfo Copy;
            Copy.TransferSize = Size;
            Copy.pSrc = &TransferBuffer;
            Copy.srcOffset = TransferTail;
            Copy.pDst = pDst;
            Copy.dstOffset = dstOffset;
            
            TransferTail += Size;
            
            BufferCopies.push(Copy);

            bTransferNeeded = true;
        }

        void Device::Copy(VkPipelineStageFlags DstStage, VkImage* pDst, void* pData, size_t Size, VkImageSubresourceLayers SubResource, VkExtent3D RegionExtent, VkOffset3D RegionOffset)
        {
			if(bTransfering)
			{
				// std::cout << "made a copy call during an active transfer.\n";
			}

            // if not allocated already, create a 192000000 volume for transfer operations
            if(TransferAlloc == nullptr)
            {
                VkBufferCreateInfo BufferCI{};
                BufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                BufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                BufferCI.size = 192000000;
                BufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                
                if(vkCreateBuffer(vkDevice, &BufferCI, nullptr, &TransferBuffer) != VK_SUCCESS)
                {
                    throw std::runtime_error("Wrappers : failed to allocate transfer buffer for copy()\n");
                }

				TransferAlloc = Allocate(TransferBuffer, Enums::eHost);
                
                pTransferMemory = TransferAlloc->Map();
            }
            
            memcpy(((uint8_t*)pTransferMemory)+TransferTail, pData, Size);
            
            ImageCopyInfo Copy;
            Copy.TransferSize = Size;
            Copy.pSrc = &TransferBuffer;
            Copy.srcOffset = TransferTail;
            
            Copy.pDst = pDst;
            Copy.SubResource = SubResource;
            Copy.Extent = RegionExtent;
            Copy.Offset = RegionOffset;
            
            Copy.Dst = DstStage;
            
            TransferTail += Size;
            
            ImgCopies.push(Copy);
            
            bTransferNeeded = true;
        }

        void Device::SubmitCopies()
        {
			// if our transfer commands are not yet created, create them.
            if(cmdTransferPool == nullptr)
            {
                cmdTransferPool = CreateCommandPool(Enums::eTransfer);
                cmdTransferBuffer = cmdTransferPool->CreateBuffer(Enums::ePrimary);
            }
            
            cmdTransferBuffer->Begin();
            
            while(BufferCopies.size() != 0)
            {
                BufferCopyInfo& CopyInfo = BufferCopies.front();

                VkBufferCopy vkCopyInfo{};
                vkCopyInfo.size = CopyInfo.TransferSize;
                vkCopyInfo.srcOffset = CopyInfo.srcOffset;
                vkCopyInfo.dstOffset = CopyInfo.dstOffset;

                vkCmdCopyBuffer(*cmdTransferBuffer, *CopyInfo.pSrc, *CopyInfo.pDst, 1, &vkCopyInfo);
				
				BufferCopies.pop(); // We pop the element out of the queue once we've read all the info we need.
            }

            // Ensure that all the buffer copies before we work on images, just in case one the buffers is used to store raw image data that will be copied into an image
            vkCmdPipelineBarrier(*cmdTransferBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
            
            while(ImgCopies.size() != 0)
            {
                ImageCopyInfo& CopyInfo = ImgCopies.front();
                
                VkBufferImageCopy vkCopyInfo{};
                vkCopyInfo.imageExtent = CopyInfo.Extent;
                vkCopyInfo.imageOffset = CopyInfo.Offset;
                vkCopyInfo.bufferOffset = CopyInfo.srcOffset;
                
                vkCopyInfo.imageSubresource = CopyInfo.SubResource;
                
                vkCmdCopyBufferToImage(*cmdTransferBuffer, *CopyInfo.pSrc, *CopyInfo.pDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vkCopyInfo);
				
				ImgCopies.pop(); // We pop the element out of the queue once we've read all the info we need.
            }
            
            vkCmdPipelineBarrier(*cmdTransferBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
            
            cmdTransferBuffer->End();
			
			// Reset and use the transfer fence and signal it when we finish transferring.
			vkResetFences(vkDevice, 1, *pTransferFence);
            cmdTransferBuffer->Signal(pTransferFence);

            // vkWaitForFences(vkDevice, 1, *pRenderFence, VK_TRUE, UINT64_MAX);
            
			cmdTransferBuffer->Submit();

			// Reset our transfer variables.
			bTransfering = true;
        }

        Swapchain* Device::CreateSwapchain(VkExtent2D SwapchainSize)
        {
            Swapchain* Ret = new Swapchain(PhysicalDevice, &vkDevice, Memory.LocalMemory, Surface, cmdGraphicsPool->CreateBuffer(Enums::ePrimary), SwapchainSize);
            
            pSwapchain = Ret;
            
            return pSwapchain;
        }

        Renderpass* Device::CreateRenderpass(uint32_t PassCount, Pass* pPasses, FrameBufferDescription FBDesc, VkSampleCountFlagBits SampleCount)
        {
            Renderpass* Ret = new Renderpass(vkDevice, PassCount, pPasses, FBDesc, SampleCount);
            
            return Ret;
        }

        CommandPool* Device::CreateCommandPool(Enums::CommandType Type)
        {
            CommandPool* Ret;
            
            if(Type == Enums::eGraphics)
            {
                Ret = new CommandPool(vkDevice, &Queues.GraphicsQueue, Families.GraphicsFamily);
                Ret->Type = Enums::eGraphics;
            }
            else if(Type == Enums::eCompute)
            {
                Ret = new CommandPool(vkDevice, &Queues.ComputeQueue, Families.ComputeFamily);
                Ret->Type = Enums::eCompute;
            }
            else if(Type == Enums::eTransfer)
            {
                Ret = new CommandPool(vkDevice, &Queues.TransferQueue, Families.TransferFamily);
                Ret->Type = Enums::eTransfer;
            }
            
            return Ret;
        }

        Fence* Device::CreateFence(std::string FenceName)
        {
            return new Fence(&vkDevice, false, FenceName);
        }

        Semaphore* Device::CreateSemaphore(std::string SemName)
        {
            return new Semaphore(&vkDevice, SemName);
        }

        Pipeline* Device::CreatePipeline(PipelineDescription PipeDesc, std::string PipeName)
        {
            PipeDesc.Descriptors.insert(PipeDesc.Descriptors.begin(), pSceneDescriptors[0]);
            
            return new Pipeline(&vkDevice, PipeDesc, PipeName);
        }


        CommandBuffer* Device::BeginRender(Swapchain* pSwapchain, Renderpass* RP)
        {
            if(bTransferNeeded)
            {
                // Execute copies on the GPU side while we spend time recording. This can be efficient if we are iterating over large numbers of meshes for one draw call.
                SubmitCopies();
                bTransferNeeded = false;
            }
            
            if(pRenderBuffer == nullptr)
            {
                pRenderBuffer = cmdGraphicsPool->CreateBuffer(Enums::ePrimary);
            }
            
            pRenderpass = RP;
            
            Framebuffer* pFB = pSwapchain->GetNextFramebuffer(FramebufferIdx);
            
            if(FramebufferIdx > 2)
            {
                std::cout << "Vulkan Gave us an invalid framebuffer index?\n";
            }
            
            VkExtent2D rpExtent = pSwapchain->GetExtent();
            
            // Wait for previous render to finish
            pRenderFence->Wait();
            
            // Update scene descriptor set
            pSceneDescriptors[(FramebufferIdx+1)%3]->Update();
            
            // reset render fence
            pRenderBuffer->Reset();
            
            pRenderBuffer->Begin();
            pRenderpass->Begin(*pRenderBuffer, pFB, rpExtent);
            
            return pRenderBuffer;
        }

        void Device::EndRender()
        {
            pRenderpass->End(*pRenderBuffer);
        }

        void Device::SubmitRender()
        {
            pRenderBuffer->End();
            
            if(bTransfering)
            {
                // basically just Purge the transfer queue so that all resources are up to date, we will fix this later to specify if the resource is used in any pipelines, if so then we create a seperate buffer and bind it to memory. We then transfer to that new buffer, once that transfer's done, we use the new buffer in place of the pDst Buffer. In short we will double-buffer resources later
                vkWaitForFences(vkDevice, 1, *pTransferFence, VK_TRUE, UINT64_MAX);
                vkResetFences(vkDevice, 1, *pTransferFence);
				TransferTail = 0;
                bTransfering = false;
            }
            
            pRenderBuffer->Signal(pRenderFence);
            pRenderBuffer->Submit();
        }

        void Device::BindScene(VkCommandBuffer* pCmdBuffer, VkPipelineLayout Layout)
        {
            pSceneDescriptors[FramebufferIdx]->Update();
            pSceneDescriptors[FramebufferIdx]->Bind(pCmdBuffer, Layout);
        }

        void Device::Present(uint32_t WaitSemCount, Semaphore* pWaitSemaphores)
        {
            std::vector<VkSemaphore> Semaphores(WaitSemCount);
            
            for(uint32_t i = 0; i < WaitSemCount; i++)
            {
                Semaphores[i] = pWaitSemaphores[i];
            }
            
            VkPresentInfoKHR PresentInfo{};
            PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            PresentInfo.swapchainCount = 1;
            PresentInfo.pSwapchains = *pSwapchain;
            PresentInfo.pImageIndices = &FramebufferIdx;
            PresentInfo.waitSemaphoreCount = Semaphores.size();
            PresentInfo.pWaitSemaphores = Semaphores.data();
            
            vkQueuePresentKHR(Queues.GraphicsQueue, &PresentInfo);
        }
    }
}
