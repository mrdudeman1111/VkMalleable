#include "Mesh.hpp"

#include <GLFW/glfw3.h>
#include <csignal>
#include <queue>
#include <vulkan/vulkan_core.h>

namespace Wrappers
{
  /* Command Buffer allocation wrapper and Shaders::DescriptorSet allocator */
    class CommandPool;
    class DescriptorAllocator; // creates descriptor sets which create ShaderBuffers

  /* Info structures */
    struct DeviceQueues;
    struct DeviceMemory;
    struct QueueFamilies;
    struct BufferCopyInfo;
    struct ImageCopyInfo;

  /* Vulkan object wrappers */
  class Device;
  class Instance;
}

namespace Wrappers
{ 
  class CommandPool : VulkanObject
  {
    public:
      CommandPool(VkDevice& Device, VkQueue* pQueue, uint32_t QueueFamily);
      ~CommandPool();

      CommandBuffer* CreateBuffer(Enums::CommandLevel cmdLevel);

      Enums::CommandType Type;

    private:
      VkCommandPool vkPool;
      VkQueue* cmdQueue;
  };

  class DescriptorAllocator : VulkanObject
  {
    public:
      DescriptorAllocator(VkDevice* Device, Memory::Allocator* pAllocator, std::string AllocatorName = "Descriptor allocator");

      Shaders::DescriptorSet* AllocateDescriptorSet(uint32_t SetIndex);

    private:
      Memory::Allocator* pAllocator;
      VkDescriptorPool vkPool;
  };

  struct DeviceQueues
  {
    VkQueue GraphicsQueue = VK_NULL_HANDLE;
    VkQueue ComputeQueue = VK_NULL_HANDLE;
    VkQueue TransferQueue = VK_NULL_HANDLE;
  };

  struct DeviceMemory
  {
    uint32_t LocalMemory;
    uint32_t VisibleMemory;
  };

  struct QueueFamilies
  {
    uint32_t GraphicsFamily;
    uint32_t ComputeFamily;
    uint32_t TransferFamily;
  };

  struct BufferCopyInfo
  {
    VkDeviceSize TransferSize;

    VkBuffer* pSrc;
    VkDeviceSize srcOffset;

    VkBuffer* pDst;
    VkDeviceSize dstOffset;
  };

  struct ImageCopyInfo
  {
    VkDeviceSize TransferSize;

    VkImageSubresourceLayers SubResource;

    VkBuffer* pSrc;
    VkDeviceSize srcOffset;

    VkImage* pDst;
    VkExtent3D Extent;
    VkOffset3D Offset;

    VkPipelineStageFlags Dst;
  };

  class Device : Memory::Allocator
  {
    public:

      /* \brief Public constructor for Device wrapper class.
       *
       * Constructor fills the device memory and queue info structures. Also create VkDevice and graphics, compute, and transfer VkQueue(s).
       */
      Device(VkPhysicalDevice PhysDev, VkSurfaceKHR RenderSurface, uint32_t ExtensionCount, const char** pExtensions, VkPhysicalDeviceFeatures Feat);

      /* \brief Public Destructor for Device wrapper class.
       *
       * Destructor destroys VkDevice along with associated VkQueue(s).
       * Be sure to delete any resources created from this object.
       */
      ~Device();

      operator Memory::Allocator*() {
        return (Memory::Allocator*)this;
      }

      operator VkDevice() {
        return vkDevice;
      }

      operator VkDevice*() {
        return &vkDevice;
      }

      operator VkPhysicalDevice() {
        return PhysicalDevice;
      }

      operator VkPhysicalDevice*() {
        return &PhysicalDevice;
      }

      /************* Inherited by Allocator ****************/

        /* \brief Create a Buffer
         *
         *  Returns a pointer to the created buffer.
         *
         *  @param Size The size in bytes of the desired buffer.
         *  @param bVisibe Whether or not this buffer should be mappable (On host memory).
         *  @param Usage The vulkan usage flags to create the buffer with.
         */
        vulkan::Buffer* CreateBuffer(size_t Size, VkBufferUsageFlags Usage = 0);

        /*  \brief Create an Image
         *
         *  Returns a pointer to the create Image.
         *
         *  @param Size The Resolution of the image to create.
         *  @param Format The Image format for the image.
         *  @param Usage The vulkan usage flags to create the image with.
         */
        vulkan::Texture* CreateImage(VkExtent2D Size, VkFormat Format, VkImageUsageFlags Usage = 0, VkSampleCountFlagBits = VK_SAMPLE_COUNT_1_BIT);

        /* \brief Allocate VkBuffer 
         *
         * Returns pointer to heap-allocated Allocation Handle on requested memory type. Can be destroyed/deleted using delete keyword.
         *
         * @param Buffer Vulkan buffer handle to allocate.
         * @param MemType Any value from the Wrappers::Enums::eMemoryType enum
         */
        Memory::Allocation* Allocate(VkBuffer& Buffer, Enums::eMemoryType MemType);

        /* \brief Allocate Buffer
         *
         *  Allocates a buffer to memory.
         *
         *  @param Buffer vulkan::Buffer pointer to allocate, must have a valid vkBuffer.
         *  @param MemType An Enum::eMemoryType value indicating the type of memory to use.
         */
        void Allocate(vulkan::Buffer* pBuffer, Enums::eMemoryType MemType);

        /* \brief Allocate VkImage
         *
         * Returns pointer to heap-allocated Allocation Handle on requested memory type. Can be destroyed/deleted using delete keyword.
         *
         * @param Image Vulkan Image handle to allocate.
         * @param MemType Any value from the wrappers::Enums::eMemoryType enum
         */
        Memory::Allocation* Allocate(VkImage& Image, Enums::eMemoryType MemType);

        /* \brief Copy Data from pointer to Vulkan Buffer
         *
         * Copies (Size) bytes from pData to global staging buffer, to be copied to pDst (which should be on device local memory, otherwise you can map the buffer directly and you won't need to use this function). The Copy is NOT immediately executed, it will execute before the next frame.
         *
         * @param DstStage the Pipeline stage in which this resource is used
         */
        void Copy(VkBuffer* pDst, void* pData, size_t Size, VkDeviceSize dstOffset = 0);

        void Copy(VkPipelineStageFlags DstStage, VkImage* pDst, void* pData, size_t Size, VkImageSubresourceLayers SubResource, VkExtent3D RegionExtent, VkOffset3D RegionOffset = {0, 0, 0});

      inline VkDevice GetDevice() { return vkDevice; }
      inline VkPhysicalDevice GetPhysicalDevice() { return PhysicalDevice; }
      inline QueueFamilies GetQueueFamilies() { return Families; }
      inline DeviceMemory GetDeviceMemory() { return Memory; }
      inline DeviceQueues GetDeviceQueues() { return Queues; }
      inline VkPhysicalDeviceFeatures GetDeviceFeatures() { return Features; }

      /* Create Methods */
        Swapchain* CreateSwapchain(VkExtent2D SwapchainSize);
        Renderpass* CreateRenderpass(uint32_t PassCount, Pass* pPasses, FrameBufferDescription FBDesc, VkSampleCountFlagBits SampleCount);
        CommandPool* CreateCommandPool(Enums::CommandType Type);
        Fence* CreateFence(std::string FenceName);
        Semaphore* CreateSemaphore(std::string SemName);
        Pipeline* CreatePipeline(PipelineDescription PipeDescription, std::string PipeName);
        Mesh::Mesh* CreateMesh();

      // Creates a scene-wide uniform buffer
      template<typename T>
      Shaders::ShaderBuffer<T>* CreateGlobalUniformBuffer(Enums::eShaderStage Stage, uint32_t Binding)
      {
        VkResult Err;

        if(pSceneDescriptors[0] == nullptr)
        {
          pSceneDescriptors[0] = pDescriptorAllocator->AllocateDescriptorSet(0);
          pSceneDescriptors[1] = pDescriptorAllocator->AllocateDescriptorSet(0);
          pSceneDescriptors[2] = pDescriptorAllocator->AllocateDescriptorSet(0);
        }

        // create a shader buffer.
        Shaders::ShaderBuffer<T>* Ret = pSceneDescriptors[0]->CreateUniformBuffer<T>(Stage, Binding);

        // add this shader buffer to the other two descriptor buffers, eventually when we implement double/triple buffering, each descriptorset will have a uniform of it's own.
        pSceneDescriptors[1]->AddUniform(Ret);
        pSceneDescriptors[2]->AddUniform(Ret);

        return Ret;
      }

      CommandBuffer* BeginRender(Swapchain* pSwapchain, Renderpass* RP);
      inline uint32_t GetFbIdx() { return FramebufferIdx; }
      void EndRender();
      void SubmitRender();

      void BindScene(VkCommandBuffer* pCmdBuffer, VkPipelineLayout Pipeline);

      // Presentation wrapper
      void Present(uint32_t WaitSemCount = 0, Semaphore* pWaitSemaphores = nullptr);

      void SubmitCopies();

    private:
      // the vulkan Device
        VkPhysicalDevice PhysicalDevice; //! < vulkan Physical Device handle
        VkDevice vkDevice; //! < vulkan device handle

      std::vector<const char*> Extensions;

      // Device info
        QueueFamilies Families;
        DeviceMemory Memory;
        DeviceQueues Queues;
        VkPhysicalDeviceFeatures Features;


      // Transfer Variables
        CommandPool* cmdTransferPool; //! < Command pool for all transfer command buffers
        CommandBuffer* cmdTransferBuffer; //! < Main command buffer for transfer commands submited via Copy() calls

        Memory::Allocation* TransferAlloc; //! < Memory Allocation Global staging buffer
        VkBuffer TransferBuffer; //! < Global staging buffer for all Copy() operations

        uint32_t TransferTail = 0; //! < Variable used to track how much memory in the staging tansfer is currently used. Resets after all copy commands have executed.

        void* pTransferMemory = nullptr; //! < pointer to stagingbuffer memory

        std::queue<BufferCopyInfo> BufferCopies; //! < All the Buffer copy commands passed through Copy() that have not yet executed

        std::queue<ImageCopyInfo> ImgCopies; //! < All the Image copy commands passed through Copy() that have not yet executed

        Fence* pTransferFence = nullptr; //! < Fence used to check if the transfer command buffer is in flight

      // We need the vulkan handles for a couple of functions used in renderpass creation
        VkSurfaceKHR Surface; //! < Handle to rendering surface.

        Swapchain* pSwapchain; //! < Pointer to main Swapchain, used during Renderpass creation.

      // Rendering
        CommandPool* cmdGraphicsPool = nullptr; //! < Command pool for all render buffers
        CommandBuffer* pRenderBuffer = nullptr; //! < Main Command buffer for rendering
        DescriptorAllocator* pDescriptorAllocator = nullptr; //! < The descriptor allocator for scene descriptor sets
        Shaders::DescriptorSet* pSceneDescriptors[3] = {nullptr, nullptr, nullptr}; //! < The descriptor sets containing all scene descriptors. This is an array with a size of 3, one descriptor set for each frame.
        bool bTransfering = false; //! < Indicates whether or not there are transfer commands in flight
        bool bTransferNeeded = false;
        Fence* pRenderFence = nullptr; //! < Fence signaled when rendering is finished

        uint32_t FramebufferIdx;

        Renderpass* pRenderpass; //! < pointer to current renderpass
  };

  class Instance
  {
    public:
      Instance();
      ~Instance();

      operator VkInstance() {
        return vkInstance;
      }

      operator VkInstance*() {
        return &vkInstance;
      }

      inline GLFWwindow* GetWindow() { return Window; }

      Device* CreateDevice(uint32_t ExtensionCount, const char** pExtensions);
      void CreateWindow(uint32_t Width, uint32_t Height, const char* WindowName);

      inline bool ShouldClose() { return glfwWindowShouldClose(Window); }
      inline void PollWindow() { glfwPollEvents(); }

    private:
      VkInstance vkInstance;

      std::vector<const char*> Layers;
      std::vector<const char*> Extensions;

      GLFWwindow* Window;
      VkSurfaceKHR Surface;
  };
};

