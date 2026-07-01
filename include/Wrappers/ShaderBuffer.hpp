#pragma once

#include "Base/Buffer.hpp"

namespace VkMall
{
    namespace Wrappers
    {
        struct ShaderBuffer_Base : VulkanObject
        {
            friend class DescriptorSet;

        public:
            ShaderBuffer_Base(VkDevice* Device, Base::Allocator* pAllocator, uint32_t Binding, Enums::eShaderStage Stage, std::string BufferName) : pAllocator(pAllocator), Binding(Binding), ShaderStage(Stage), VulkanObject(Device, BufferName) {};
            ~ShaderBuffer_Base() 
            {
            }

            uint32_t GetBinding() { return Binding; }

            virtual void Update() = 0;
            
            Enums::eShaderStage ShaderStage;

        protected:
            Base::Allocator* pAllocator;
            uint32_t Binding;

        private:
            virtual VkDescriptorBufferInfo DescriptorWrite() = 0;

        };

        template<typename T>
        struct ShaderBuffer : ShaderBuffer_Base
        {
        public:
            ShaderBuffer(VkDevice* Device, Base::Allocator* pAllocator, uint32_t Binding, Enums::eShaderStage ShaderStage, std::string BufferName = "Shader Buffer") : ShaderBuffer_Base(Device, pAllocator, Binding, ShaderStage, BufferName)
            {
                pBuffer = new T(); // This is just a generic memory block on system memory (RAM). We use this as our staging region, so during Update() we make a call to the allocator to transfer the contents of this pointer to the actual buffer we use for shaders, pLocalBuffer.

                if((pLocalBuffer = pAllocator->CreateBuffer(sizeof(T), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)) == nullptr)
                {
                    throw std::runtime_error("wrappers : " + BufferName + "'s constructor failed to create device local buffer\n");
                }

                VkBuffer Buff = *pLocalBuffer;

                pLocalAlloc = pAllocator->Allocate(Buff, Enums::eLocal);
            }

            ~ShaderBuffer()
            {
                delete pBuffer;
                delete pLocalBuffer;
                delete pLocalAlloc;
            }

            void Update()
            {
                // push staging to local buffer
                pAllocator->Copy(*pLocalBuffer, pBuffer, sizeof(T));
            }

            VkDescriptorBufferInfo DescriptorWrite()
            {
                VkDescriptorBufferInfo BuffInfo{};
                BuffInfo.range = pLocalAlloc->Size;
                BuffInfo.buffer = *pLocalBuffer;
                BuffInfo.offset = 0;

                return BuffInfo;
            }

            T* pBuffer;

            Base::Buffer* pLocalBuffer;
            Base::Allocation* pLocalAlloc;
        };
    }
}
