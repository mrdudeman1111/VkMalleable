#include "Wrappers/Descriptors.hpp"

namespace VkMall
{
    namespace Wrappers
    {
        DescriptorSet::DescriptorSet(VkDevice* Device, VkDescriptorPool* pDescriptorPool, Base::Allocator* pAllocator, uint32_t SetIndex, std::string DescriptorName) : pAllocator(pAllocator), pPool(pDescriptorPool), SetIndex(SetIndex), VulkanObject(Device, DescriptorName)
        {
            vkSet = VK_NULL_HANDLE;
        }

        void DescriptorSet::AddUniform(ShaderBuffer_Base* Uniform)
        {
            VkResult Err;

            for(uint32_t i = 0; i < Bindings.size(); i++)
            {
                if(Uniform->GetBinding() == Bindings[i].binding)
                {
                std::cout << "Wrappers : " << ObjectName << " : Attempted to add a uniform buffer (" << Uniform->ObjectName << ") which shares a binding index with one of the descriptors\n";
                return;
                }
            }

            UniformBuffers.push_back(Uniform);

            VkDescriptorSetLayoutBinding Binding{};
            Binding.binding = Uniform->GetBinding();
            Binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            Binding.descriptorCount = 1;
            Binding.stageFlags = (VkShaderStageFlagBits)Uniform->ShaderStage;

            Bindings.push_back(Binding);

            VkDescriptorSetLayoutCreateInfo LayoutCI{};
            LayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            LayoutCI.bindingCount = Bindings.size();
            LayoutCI.pBindings = Bindings.data();

            if((Err = vkCreateDescriptorSetLayout(*pDevice, &LayoutCI, nullptr, &vkLayout)) != VK_SUCCESS)
            {
                throw std::runtime_error("Wrappers : failed to recreate descriptor set layout while adding buffer for descriptor " + ObjectName);
            }

            Dirty = true;
        }

        void DescriptorSet::Update()
        {
            VkResult Err;

            if(Dirty || vkSet == VK_NULL_HANDLE)
            {
                vkFreeDescriptorSets(*pDevice, *pPool, 1, &vkSet);

                VkDescriptorSetAllocateInfo AllocInfo{};
                AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                AllocInfo.descriptorPool = *pPool;
                AllocInfo.descriptorSetCount = 1;
                AllocInfo.pSetLayouts = &vkLayout;

                if((Err = vkAllocateDescriptorSets(*pDevice, &AllocInfo, &vkSet)) != VK_SUCCESS)
                {
                throw std::runtime_error("Wrappers : failed to update(reallocate) descriptor set : " + ObjectName);
                }
            }

            std::vector<VkDescriptorBufferInfo> BufferInfos;
            std::vector<VkWriteDescriptorSet> WriteInfos;

            for(uint32_t i = 0; i < UniformBuffers.size(); i++)
            {
                BufferInfos.push_back(UniformBuffers[i]->DescriptorWrite());

                VkWriteDescriptorSet tmpWrite{};
                tmpWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

                tmpWrite.dstSet = vkSet;
                tmpWrite.dstBinding = UniformBuffers[i]->GetBinding();
                tmpWrite.dstArrayElement = 0;

                tmpWrite.pBufferInfo = &BufferInfos[i];

                tmpWrite.descriptorCount = 1;
                tmpWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

                WriteInfos.push_back(tmpWrite);
            }

            vkUpdateDescriptorSets(*pDevice, WriteInfos.size(), WriteInfos.data(), 0, nullptr);
        }

        void DescriptorSet::Bind(VkCommandBuffer* pCmdBuffer, VkPipelineLayout Pipeline)
        {
            vkCmdBindDescriptorSets(*pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline, SetIndex, 1, &vkSet, 0, nullptr);
        }

        DescriptorAllocator::DescriptorAllocator(VkDevice* Device, Base::Allocator* pAllocator, std::string AllocatorName) : pAllocator(pAllocator), VulkanObject(Device, AllocatorName)
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

        DescriptorSet* DescriptorAllocator::AllocateDescriptorSet(uint32_t SetIndex)
        {
            // Todo : Add a check to determine whether we're over budget.
            return new DescriptorSet(pDevice, &vkPool, pAllocator, SetIndex);
        }
    }
}