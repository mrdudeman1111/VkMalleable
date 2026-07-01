#pragma once

#include "Wrappers/Pipeline.hpp"

#include "tiny_gltf.h"

#include "glm/glm.hpp"

#include <utility>

namespace VkMall
{
    namespace Wrappers
    {
		class MeshInstance
        {};

        class Mesh : VulkanObject
        {
        public:
			Mesh(VkDevice* Device, Base::Allocator* pAllocator, std::string MeshName = "Mesh") : pAllocator(pAllocator), VulkanObject(Device, MeshName)
			{
				gpuMeshBuffer = VK_NULL_HANDLE;
				MeshBufferAlloc = nullptr;

				MeshCount++;

				if(pLoader == nullptr)
				{
					pLoader = new tinygltf::TinyGLTF();
				}
			}

			~Mesh();

            inline static uint32_t MeshCount = 0;
            inline static tinygltf::TinyGLTF* pLoader = nullptr;
			
			// Creates our vertex list, must be called. Can also be used to optionally set a custom vertex class, as long as it inherits from and implements IVertexList
			template <typename T = DefaultVertex>
			void SetVertexType()
			{
				if(!std::is_base_of<IVertexList, T>::value)
				{
					throw std::runtime_error("Attempted to assemble a class with a vertex type that is NOT derived from IVertexList, this is not acceptable and T MUST inherit from IVertexList");
				}
				
				pVertices = new T();
			}
            
            /*! \brief returns the vertex information.
            *  returns a structure of type VertexBinding that specifies the binding and stride of the This Mesh's vertices.
            *  This structure does need to be passed to pipeline creation.
            */
            inline VertexBinding GetVertexBinding()
			{
				return pVertices->GetBinding();
			}

			inline VertexDescription GetVertexDescription()
			{
				return pVertices->GetDescription();
			}

            /* \brief Binds this mesh for drawing.
            *
            *  Binds this mesh's vertices, indices, and descriptor set (defaults to binding 3 usually).
            */
            inline void Bind(VkCommandBuffer* cmdBuffer)
            {
                VkDeviceSize VtxOffset = 0;
                vkCmdBindVertexBuffers(*cmdBuffer, 0, 1, &gpuMeshBuffer, &VtxOffset);
                VkDeviceSize IdxOffset = pVertices->Size();
                vkCmdBindIndexBuffer(*cmdBuffer, gpuMeshBuffer, IdxOffset, VK_INDEX_TYPE_UINT32);
            }
            
            /* \brief Draws this mesh (call Bind() first).
            *
            *
            */
            inline void Draw(VkCommandBuffer* cmdBuffer) { vkCmdDrawIndexed(*cmdBuffer, (uint32_t)Indices.size(), 1, 0, 0, 0); }
            
            /*! \brief Loads a mesh file into this object */
            bool Load(std::string MeshPath);

            /*! \brief Updates vertices, indices, and any other shader data associated with this mesh. */
            void Update();
            
            bool isLoading() { return bLoading; }
            
            glm::mat4 Transform;

            IVertexList* pVertices; //! > This is a pointer to the SoA structure containing our vertices. It contains ALL our (this instance's) vertices.
            std::vector<uint32_t> Indices;

        private:
			size_t VertexStride;
            
            Base::Allocator* pAllocator;
            
            // gpu buffer
            VkBuffer gpuMeshBuffer;
            Base::Allocation* MeshBufferAlloc;

            VertexBinding* vtxBindings = nullptr;
            
            bool bLoading;
        };
    }
}
