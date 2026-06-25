#pragma once

#include "Renderpass.hpp"

#include <cwchar>
#include <string>
#include <vulkan/vulkan_core.h>

namespace Wrappers
{
  namespace Shaders
  {
    struct Shader;
    struct ShaderBuffer_Base;
    template <typename T> struct ShaderBuffer;
    class DescriptorSet;
  }

  struct VertexDescription;
  struct VertexBinding;

  struct PipelineDescription;
  class Pipeline;
}

namespace Wrappers
{
  namespace Shaders
  {
    struct Shader : VulkanObject
    {
      public:
        Shader(VkDevice* Device, Enums::eShaderStage Stage, std::string ShaderPath, std::string ShaderName = "Shader");
        ~Shader();

        inline VkShaderModule GetShaderModule() { return vkShader; }
        inline VkPipelineShaderStageCreateInfo GetShaderStageInfo() { return StageInfo; }

        Enums::eShaderStage Stage;

      private:
        VkShaderModule vkShader;
        VkPipelineShaderStageCreateInfo StageInfo;
    };

    struct ShaderBuffer_Base : VulkanObject
    {
      friend DescriptorSet;

      public:
        uint32_t GetBinding() { return Binding; }

        virtual void Update() = 0;
        
        Enums::eShaderStage ShaderStage;

      protected:
        ShaderBuffer_Base(VkDevice* Device, Memory::Allocator* pAllocator, uint32_t Binding, Enums::eShaderStage Stage, std::string BufferName) : pAllocator(pAllocator), Binding(Binding), ShaderStage(Stage), VulkanObject(Device, BufferName) {};
        ~ShaderBuffer_Base() 
        {
        }

        Memory::Allocator* pAllocator;
        uint32_t Binding;

      private:
        virtual VkDescriptorBufferInfo DescriptorWrite() = 0;

    };

    template<typename T>
    struct ShaderBuffer : ShaderBuffer_Base
    {
      public:
        ShaderBuffer(VkDevice* Device, Memory::Allocator* pAllocator, uint32_t Binding, Enums::eShaderStage ShaderStage, std::string BufferName = "Shader Buffer") : ShaderBuffer_Base(Device, pAllocator, Binding, ShaderStage, BufferName)
        {
          pBuffer = new T();

          VkBufferCreateInfo BufferCI{};
          BufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
          BufferCI.size = sizeof(T);
          BufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
          BufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

          if(vkCreateBuffer(*pDevice, &BufferCI, nullptr, &LocalBuffer) != VK_SUCCESS)
          {
            throw std::runtime_error("Wrappers : " + BufferName + "'s Constructor failed to create Device local buffer\n");
          }

          LocalAlloc = pAllocator->Allocate(LocalBuffer, Enums::eLocal);
        }

        ~ShaderBuffer()
        {
          delete pBuffer;
          vkDestroyBuffer(*pDevice, LocalBuffer, nullptr);
          delete LocalAlloc;
        }

        void Update()
        {
          // Push Staging to Local buffer
          pAllocator->Copy(&LocalBuffer, pBuffer, sizeof(T));
        }

        VkDescriptorBufferInfo DescriptorWrite()
        {
          VkDescriptorBufferInfo BuffInfo{};
          BuffInfo.range = LocalAlloc->Size;
          BuffInfo.buffer = LocalBuffer;
          BuffInfo.offset = 0;

          return BuffInfo;
        }

        T* pBuffer;

        VkBuffer LocalBuffer;
        Memory::Allocation* LocalAlloc;
    };

    class DescriptorSet : VulkanObject
    {
      public:
        DescriptorSet(VkDevice* Device, VkDescriptorPool* pDescriptorPool, Memory::Allocator* pAllocator, uint32_t SetIndex, std::string SetName = "Descriptor Set");

        operator VkDescriptorSet() {
          return vkSet;
        }

        operator VkDescriptorSet*() {
          return &vkSet;
        }

        inline VkDescriptorSetLayout GetLayout() { return vkLayout; }

        /* \brief Bind the descriptor set to the pipeline
         *  
         *  Binds this descriptor set to the pipeline at whatever binding index it was set to at creation time (the SetIndex variable);
         *
         *  @param pCmdBuffer Pointer to the Recording vulkan command buffer.
         *  @param Pipeline The Pipeline layout, needed for vkCmdBindDescriptorSets().
         */
        void Bind(VkCommandBuffer* pCmdBuffer, VkPipelineLayout Pipeline);

        /* \brief Create a UniformBuffer and returns a handle to it.
         *
         *  @param BufferStage Specifies what shader stage this buffer binds to.
         *  @param Binding Specifies the uniform buffer's binding.
         */
        template<typename T>
        ShaderBuffer<T>* CreateUniformBuffer(Enums::eShaderStage BufferStage, uint32_t Binding)
        {
          VkResult Err;

          VkBuffer Buffer;
          Memory::Allocation* pAllocation;

          // create buffer and allocate memory
          {
            VkBufferCreateInfo BufferCI{};
            BufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            BufferCI.size = sizeof(T);
            BufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            BufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if((Err = vkCreateBuffer(*pDevice, &BufferCI, nullptr, &Buffer)) != VK_SUCCESS)
            {
              throw std::runtime_error("Wrappers : failed to create a buffer for a shader buffer in " + ObjectName);
            }

            pAllocation = pAllocator->Allocate(Buffer, Enums::eLocal);
          }

          ShaderBuffer<T>* pBuffer = new ShaderBuffer<T>(pDevice, pAllocator, Binding, BufferStage);

          VkDescriptorSetLayoutBinding LayoutBinding{};
          LayoutBinding.binding = UniformBuffers.size();
          LayoutBinding.descriptorCount = 1;
          LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

          switch(pBuffer->ShaderStage)
          {
            case Enums::eShaderStage::eVertex:
              LayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
              break;
            case Enums::eShaderStage::eFragment:
              LayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
              break;
            case Enums::eShaderStage::eGeometry:
              LayoutBinding.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
              break;
          }

          Bindings.push_back(LayoutBinding);

          UniformBuffers.push_back(pBuffer);

          VkDescriptorSetLayoutCreateInfo LayoutCI{};
          LayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
          LayoutCI.bindingCount = Bindings.size();
          LayoutCI.pBindings = Bindings.data();

          if((Err = vkCreateDescriptorSetLayout(*pDevice, &LayoutCI, nullptr, &vkLayout)) != VK_SUCCESS)
          {
            throw std::runtime_error("Wrappers : failed to recreate descriptor set layout while adding buffer for descriptor " + ObjectName);
          }

          Dirty = true;

          return pBuffer;
        }

        /*! \brief Add resource allocated from another DescriptorSet, basically duplicates the binding and element location
         *
         *  Add a shader buffer to this DescriptorSet. WILL fail and possibly crash if shaderbuffer shares a binding with an existing descriptor.
         *
         *  @param Uniform Pointer to Shader buffer to add to this set.
         */
        void AddUniform(ShaderBuffer_Base* Uniform);

        /* \brief Allocate VkBuffer 
         *
         * Resizes (through recreation) the descriptor set and updates all ShaderBuffer(s) (contained in UniformBuffers).
         */
        void Update();

      private:
        // Vulkan Descriptor objects
          VkDescriptorSetLayout vkLayout;
          VkDescriptorSet vkSet;

          bool Dirty; //! Used to check if descriptor set needs resizing.

          uint32_t SetIndex; //! > Index of descriptor, directly corresponds with layout(binding = $SetIndex) in your shader

        Memory::Allocator* pAllocator; //! > Reference to an Allocator interface for memory allocation.

        // Allocator pool
          VkDescriptorPool* pPool; //! > Reference to parent descriptor pool.

        // shader buffers/textures and binding locations
          std::vector<ShaderBuffer_Base*> UniformBuffers; //! > Contains references to all descriptors associated with this descriptor set. Descriptor binding is determined by creation order (0 = #Descriptors::size())
          std::vector<VkDescriptorSetLayoutBinding> Bindings; //! > Contains all Descriptor bindings. Used during recreation to resize the descriptor.
    };
  }

  struct VertexDescription
  {
    public:
      void AddAttribute(uint32_t AttribBinding, uint32_t AttribLocation, size_t AttribOffset, Enums::eAttributeType Type);

      inline VkVertexInputAttributeDescription* GetAttributes(uint32_t& AttribCount) { AttribCount = Attributes.size(); return Attributes.data(); }

    private:
      std::vector<VkVertexInputAttributeDescription> Attributes;

      uint32_t Offset = 0;
  };

  struct VertexBinding
  {
    public:
      inline VkVertexInputBindingDescription* GetBindings(uint32_t& Count)
      {
        Count = Bindings.size();
        return Bindings.data();
      }

      void SetStride(uint32_t Binding, uint32_t Stride)
      {
        if(Binding >= Bindings.size())
        {
          while(Binding >= Bindings.size())
          {
            Bindings.push_back({});
          }
        }

        Bindings[Binding].stride = Stride;
        Bindings[Binding].binding = 0;
        Bindings[Binding].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      }

    private:
      std::vector<VkVertexInputBindingDescription> Bindings;
  };

  struct PipelineDescription
  {
    std::string VertexShaderPath = "";
    std::string GeometryShaderPath = "";
    std::string FragmentShaderPath = "";

    VertexBinding vtxBinding;
    VertexDescription vtxDesc;

    Renderpass* pRenderpass;
    uint32_t Subpass;

    VkSampleCountFlagBits SampleCount;

    VkPrimitiveTopology Topology;

    VkExtent2D PipelineResolution;

    std::vector<Shaders::DescriptorSet*> Descriptors;
  };

  class Pipeline : VulkanObject
  {
    public:
      Pipeline(VkDevice* Device, PipelineDescription PipeDesc, std::string PipelineName = "Pipeline");
      ~Pipeline();

      operator VkPipeline() {
        return vkPipe;
      }

      operator VkPipeline*() {
        return &vkPipe;
      }

      operator VkPipelineLayout() {
        return Layout;
      }

      inline void Bind(VkCommandBuffer* cmdBuffer) { vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipe); }
      inline VkPipeline GetPipeline() { return vkPipe; }
      inline VkPipelineLayout GetLayout() { return Layout; }

    private:
      VkPipelineLayout Layout;
      VkPipeline vkPipe;

      Renderpass* RP;

      bool bUseGeometry;

      Shaders::Shader* VertexShader;
      Shaders::Shader* GeometryShader;

      Shaders::Shader* FragmentShader;
  };
}

