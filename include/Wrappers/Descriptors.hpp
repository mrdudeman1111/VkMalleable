#pragma once

#include "Wrappers/ShaderBuffer.hpp"

namespace VkMall
{
    namespace Wrappers
    {

        class DescriptorAllocator : VulkanObject
        {
            public:
            DescriptorAllocator(VkDevice* Device, Base::Allocator* pAllocator, std::string AllocatorName = "Descriptor allocator");

            class DescriptorSet* AllocateDescriptorSet(uint32_t SetIndex);

            private:
            Base::Allocator* pAllocator;
            VkDescriptorPool vkPool;
        };

        class DescriptorSet : VulkanObject
        {
        public:
            DescriptorSet(VkDevice* Device, VkDescriptorPool* pDescriptorPool, Base::Allocator* pAllocator, uint32_t SetIndex, std::string SetName = "Descriptor Set");

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
                Base::Allocation* pAllocation;

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

            /*! \brief Add resource allocated from another DescriptorSet, basically duplicates the binding and element location So you can 
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

            Base::Allocator* pAllocator; //! > Reference to an Allocator interface for memory allocation.

            // Allocator pool
            VkDescriptorPool* pPool; //! > Reference to parent descriptor pool.

            // shader buffers/textures and binding locations
            std::vector<ShaderBuffer_Base*> UniformBuffers; //! > Contains references to all descriptors associated with this descriptor set. Descriptor binding is determined by creation order (0 = #Descriptors::size())
            std::vector<VkDescriptorSetLayoutBinding> Bindings; //! > Contains all Descriptor bindings. Used during recreation to resize the descriptor.
        };
    }
}