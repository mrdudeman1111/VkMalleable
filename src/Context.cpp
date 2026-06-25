#include "Context.hpp"
#include "Base.hpp"
#include "Pipeline.hpp"

#include <cstring>
#include <pthread.h>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Wrappers
{
CommandPool::CommandPool(VkDevice& Device, VkQueue* pQueue, uint32_t QueueFamily) : cmdQueue(pQueue), VulkanObject(&Device, "CommandPool")
{
    VkCommandPoolCreateInfo PoolCI{};
    PoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    PoolCI.queueFamilyIndex = QueueFamily;
    PoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    vkCreateCommandPool(Device, &PoolCI, nullptr, &vkPool);
}

CommandPool::~CommandPool()
{
    vkDestroyCommandPool(*pDevice, vkPool, nullptr);
}

CommandBuffer* CommandPool::CreateBuffer(Enums::CommandLevel cmdLevel)
{
    VkCommandBuffer tmp;
    
    VkCommandBufferAllocateInfo AllocInfo{};
    AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    AllocInfo.commandPool = vkPool;
    AllocInfo.commandBufferCount = 1;
    AllocInfo.level = (cmdLevel == Enums::ePrimary) ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    
    vkAllocateCommandBuffers(*pDevice, &AllocInfo, &tmp);
    
    return new CommandBuffer(*pDevice, &vkPool, cmdQueue, tmp, Type);
}

DescriptorAllocator::DescriptorAllocator(VkDevice* Device, Memory::Allocator* pAllocator, std::string AllocatorName) : pAllocator(pAllocator), VulkanObject(Device, AllocatorName)
{
    VkDescriptorPoolSize Sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 20},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 50},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 20},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100}
    };
    
    VkDescriptorPoolCreateInfo PoolCI{};
    PoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    PoolCI.maxSets = 50;
    PoolCI.poolSizeCount = 5;
    PoolCI.pPoolSizes = Sizes;
    
    vkCreateDescriptorPool(*pDevice, &PoolCI, nullptr, &vkPool);
}

Shaders::DescriptorSet* DescriptorAllocator::AllocateDescriptorSet(uint32_t SetIndex)
{
    return new Shaders::DescriptorSet(pDevice, &vkPool, pAllocator, SetIndex);
}

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

vulkan::Buffer* Device::CreateBuffer(size_t Size, VkBufferUsageFlags Usage)
{
    VkResult Err;
    
    VkBufferCreateInfo BufferCI{};
    BufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCI.size = Size;
    BufferCI.usage = Usage;
    BufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    vulkan::Buffer* Ret = new vulkan::Buffer(&vkDevice);
    
    if((Err = vkCreateBuffer(vkDevice, &BufferCI, nullptr, &Ret->vkBuffer)) != VK_SUCCESS)
    {
        throw std::runtime_error("Wrappers : Failed to create buffer");
    }
    
    return Ret;
}

vulkan::Texture* Device::CreateImage(VkExtent2D Extent, VkFormat Format, VkImageUsageFlags Usage, VkSampleCountFlagBits Samples)
{
    VkResult Err;
    
    vulkan::Texture* pRet = new vulkan::Texture(&vkDevice);
    
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

Memory::Allocation* Device::Allocate(VkBuffer& Buffer, Enums::eMemoryType MemType)
{
    VkMemoryRequirements MemReq{};
    vkGetBufferMemoryRequirements(vkDevice, Buffer, &MemReq);
    
    // if the allocation is too large for a standard page.
    if(MemReq.size > PAGE_SIZE)
    {
        Memory::Page* pTmp = new Memory::Page(&vkDevice);
        
        // Configuration for a specialized page large enough to handle the allocation size.
        VkMemoryAllocateInfo AllocInfo{};
        AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocInfo.allocationSize = MemReq.size;
        AllocInfo.memoryTypeIndex = (MemType == Enums::eHost) ? Memory.VisibleMemory : Memory.LocalMemory;
        
        if(vkAllocateMemory(vkDevice, &AllocInfo, nullptr, &pTmp->Memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Wrappers : failed to allocate memory page for host-visible allocation");
        }
        
        Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);
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
                Memory::Allocation* Iterator = HostVisiblePages[i]->HeadAllocation;
                
                while(Iterator)
                {
                    if(Iterator->pNext)
                    {
                        if(Iterator->pNext->Offset - (Iterator->Offset+Iterator->Size) > MemReq.size+32)
                        {
                            // find allocation position in memory and align with alignment requirements
                            uint32_t AllocPosition = Iterator->Offset+Iterator->Size;
                            AllocPosition += MemReq.alignment-(AllocPosition%MemReq.alignment);
                            
                            Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);
                            
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
                            
                            Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);

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
        
        Memory::Page* newPage = new Memory::Page(&vkDevice);
        
        VkMemoryAllocateInfo AllocInfo{};
        AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocInfo.allocationSize = PAGE_SIZE;
        AllocInfo.memoryTypeIndex = Memory.VisibleMemory;
        
        if(vkAllocateMemory(vkDevice, &AllocInfo, nullptr, &newPage->Memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Wrappers : failed to allocate memory for new host visible page\n");
        }
        
        Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);
        
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
                Memory::Allocation* Iterator = DeviceLocalPages[i]->HeadAllocation;
                
                while(Iterator)
                {
                    if(Iterator->pNext)
                    {
                        if(Iterator->pNext->Offset - (Iterator->Offset+Iterator->Size) > MemReq.size+32)
                        {
                            // find allocation position in memory and align with alignment requirements
                            uint32_t AllocPosition = Iterator->Offset+Iterator->Size;
                            AllocPosition += MemReq.alignment-(AllocPosition%MemReq.alignment);
                            
                            Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);
                            
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
                            
                            Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);
                            
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
        
        Memory::Page* newPage = new Memory::Page(&vkDevice);
        
        VkMemoryAllocateInfo AllocInfo{};
        AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocInfo.allocationSize = PAGE_SIZE;
        AllocInfo.memoryTypeIndex = Memory.LocalMemory;
        
        if(vkAllocateMemory(vkDevice, &AllocInfo, nullptr, &newPage->Memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Wrappers : failed to allocate memory for new host visible page\n");
        }
        
        Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);
        
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

void Device::Allocate(vulkan::Buffer* pBuffer, Enums::eMemoryType MemType)
{
    pBuffer->Alloc = Allocate(pBuffer->vkBuffer, MemType);
    return;
}

Memory::Allocation* Device::Allocate(VkImage& Image, Enums::eMemoryType MemType)
{
    VkMemoryRequirements MemReq{};
    vkGetImageMemoryRequirements(vkDevice, Image, &MemReq);
    
    if(MemReq.size > PAGE_SIZE)
    {
        Memory::Page* pTmp = new Memory::Page(&vkDevice);
        
        VkMemoryAllocateInfo AllocInfo{};
        AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocInfo.allocationSize = MemReq.size;
        AllocInfo.memoryTypeIndex = (MemType == Enums::eHost) ? Memory.VisibleMemory : Memory.LocalMemory;
        
        if(vkAllocateMemory(vkDevice, &AllocInfo, nullptr, &pTmp->Memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Wrappers : failed to allocate memory page for host-visible allocation");
        }
        
        Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);
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
                Memory::Allocation* Iterator = HostVisiblePages[i]->HeadAllocation;
                
                while(Iterator)
                {
                    if(Iterator->pNext)
                    {
                        if(Iterator->pNext->Offset - (Iterator->Offset+Iterator->Size) > MemReq.size+32)
                        {
                            // find allocation position in memory and align with alignment requirements
                            uint32_t AllocPosition = Iterator->Offset+Iterator->Size;
                            AllocPosition = AllocPosition+(AllocPosition%MemReq.alignment);
                            
                            Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);
                            
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
                            
                            Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);
                            
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
        
        Memory::Page* newPage = new Memory::Page(&vkDevice);
        
        VkMemoryAllocateInfo AllocInfo{};
        AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocInfo.allocationSize = PAGE_SIZE;
        AllocInfo.memoryTypeIndex = Memory.VisibleMemory;
        
        if(vkAllocateMemory(vkDevice, &AllocInfo, nullptr, &newPage->Memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Wrappers : failed to allocate memory for new host visible page\n");
        }
        
        Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);
        
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
                Memory::Allocation* Iterator = DeviceLocalPages[i]->HeadAllocation;
                
                while(Iterator)
                {
                    if(Iterator->pNext)
                    {
                        if(Iterator->pNext->Offset - (Iterator->Offset+Iterator->Size) > MemReq.size+32)
                        {
                            // find allocation position in memory and align with alignment requirements
                            uint32_t AllocPosition = Iterator->Offset+Iterator->Size;
                            AllocPosition = AllocPosition+(AllocPosition%MemReq.alignment);
                            
                            Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);
                            
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
                            
                            Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);
                            
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
        
        Memory::Page* newPage = new Memory::Page(&vkDevice);
        
        VkMemoryAllocateInfo AllocInfo{};
        AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocInfo.allocationSize = PAGE_SIZE;
        AllocInfo.memoryTypeIndex = Memory.LocalMemory;
        
        if(vkAllocateMemory(vkDevice, &AllocInfo, nullptr, &newPage->Memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Wrappers : failed to allocate memory for new host visible page\n");
        }
        
        Memory::Allocation* newAlloc = new Memory::Allocation(&vkDevice, (Memory::Allocator*)this);
        
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
    }
    
    if(pTransferMemory == nullptr)
    {
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
    }
    
    if(pTransferMemory == nullptr)
    {
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
    if(cmdTransferPool == nullptr)
    {
        cmdTransferPool = CreateCommandPool(Enums::eTransfer);
        cmdTransferBuffer = cmdTransferPool->CreateBuffer(Enums::ePrimary);
    }
    
    cmdTransferBuffer->Begin();
    
    while(BufferCopies.size() != 0)
    {
        BufferCopyInfo& CopyInfo = BufferCopies.front();
        BufferCopies.pop();
        
        VkBufferCopy vkCopyInfo{};
        vkCopyInfo.size = CopyInfo.TransferSize;
        vkCopyInfo.srcOffset = CopyInfo.srcOffset;
        vkCopyInfo.dstOffset = CopyInfo.dstOffset;
        
        vkCmdCopyBuffer(*cmdTransferBuffer, *CopyInfo.pSrc, *CopyInfo.pDst, 1, &vkCopyInfo);
    }
    
    // Ensure that all the buffer copies before we work on images, just in case one the buffers is used to store raw image data that will be copied into an image
    vkCmdPipelineBarrier(*cmdTransferBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
    
    while(ImgCopies.size() != 0)
    {
        ImageCopyInfo& CopyInfo = ImgCopies.front();
        ImgCopies.pop();
        
        VkBufferImageCopy vkCopyInfo{};
        vkCopyInfo.imageExtent = CopyInfo.Extent;
        vkCopyInfo.imageOffset = CopyInfo.Offset;
        vkCopyInfo.bufferOffset = CopyInfo.srcOffset;
        
        vkCopyInfo.imageSubresource = CopyInfo.SubResource;
        
        vkCmdCopyBufferToImage(*cmdTransferBuffer, *CopyInfo.pSrc, *CopyInfo.pDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vkCopyInfo);
    }
    
    vkCmdPipelineBarrier(*cmdTransferBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
    
    cmdTransferBuffer->End();
    
    vkResetFences(vkDevice, 1, *pTransferFence);
    
    cmdTransferBuffer->Signal(pTransferFence);
    
    vkWaitForFences(vkDevice, 1, *pRenderFence, VK_TRUE, UINT64_MAX);
    
    cmdTransferBuffer->Submit();
    
    bTransfering = true;
    
    TransferTail = 0;
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

Mesh::Mesh* Device::CreateMesh()
{
    Mesh::Mesh* Ret = new Mesh::Mesh(&vkDevice, (Memory::Allocator*)this);
    
    return Ret;
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

Instance::Instance()
{
    VkResult Err;
    
    glfwInit();
    
    uint32_t ExtCount;
    const char** Exts = glfwGetRequiredInstanceExtensions(&ExtCount);
    
    for(uint32_t i = 0; i < ExtCount; i++)
    {
        Extensions.push_back(Exts[i]);
    }
    
    Layers.push_back("VK_LAYER_KHRONOS_validation");
    
    Extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    
#ifdef __APPLE__
    Extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
    
    VkInstanceCreateInfo InstCI{};
    InstCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef __APPLE__
    InstCI.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    InstCI.enabledLayerCount = (uint32_t)Layers.size();
    InstCI.ppEnabledLayerNames = Layers.data();
    InstCI.enabledExtensionCount = (uint32_t)Extensions.size();
    InstCI.ppEnabledExtensionNames = Extensions.data();
    
    if((Err = vkCreateInstance(&InstCI, nullptr, &vkInstance)) != VK_SUCCESS)
    {
        throw std::runtime_error("Wrappers: Failed to create instance");
    }
    
    Window = nullptr;
}

Instance::~Instance()
{
    vkDestroySurfaceKHR(vkInstance, Surface, nullptr);
    glfwDestroyWindow(Window);
    glfwTerminate();
    
    vkDestroyInstance(vkInstance, nullptr);
}

Device* Instance::CreateDevice(uint32_t ExtensionCount, const char** pExtensions)
{
    VkPhysicalDevice PhysDev = VK_NULL_HANDLE;
    VkDevice Dev = VK_NULL_HANDLE;
    
    uint32_t PhysDevCount;
    vkEnumeratePhysicalDevices(vkInstance, &PhysDevCount, nullptr);
    std::vector<VkPhysicalDevice> PhysicalDevices(PhysDevCount);
    vkEnumeratePhysicalDevices(vkInstance, &PhysDevCount, PhysicalDevices.data());
    
    for(uint32_t i = 0; i < PhysDevCount; i++)
    {
        VkPhysicalDeviceProperties DevProps;
        vkGetPhysicalDeviceProperties(PhysicalDevices[i], &DevProps);
        
        if(DevProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            PhysDev = PhysicalDevices[i];
        }
    }
    
    if(PhysDev == VK_NULL_HANDLE)
    {
        for(uint32_t i = 0; i < PhysicalDevices.size(); i++)
        {
            VkPhysicalDeviceProperties DevProps;
            vkGetPhysicalDeviceProperties(PhysicalDevices[i], &DevProps);
            
            if(DevProps.deviceType & VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU | VK_PHYSICAL_DEVICE_TYPE_CPU)
            {
                PhysDev = PhysicalDevices[i];
            }
        }
    }
    
    VkPhysicalDeviceFeatures DevFeat;
    vkGetPhysicalDeviceFeatures(PhysDev, &DevFeat);
    
    Device* Ret = new Device(PhysDev, Surface, ExtensionCount, pExtensions, DevFeat);
    
    return Ret;
}

void Instance::CreateWindow(uint32_t Width, uint32_t Height, const char* WindowName)
{
    if(Window != nullptr)
    {
        return;
    }
    
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    Window = glfwCreateWindow(Width, Height, WindowName, NULL, NULL);
    
    glfwCreateWindowSurface(vkInstance, Window, nullptr, &Surface);
}
}

